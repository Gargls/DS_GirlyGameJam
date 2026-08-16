// SPDX-License-Identifier: CC0-1.0
//
// Looping background music, streamed from NitroFS.
//
// The two tracks are full songs (over two minutes each), not tracker modules,
// so mmutil's soundbank route does not fit -- that bakes samples whole into
// the ROM and expects them to fit in main RAM when played. Streaming reads a
// chunk at a time instead, the way BlocksDS's own maxmod/streaming example
// does: a circular staging buffer is topped up from the file in the main
// loop, and maxmod's callback (which runs in an interrupt, where file IO is
// not safe) only ever copies out of that buffer.
//
// The staging buffer is filled from a real WAV file, and a WAV's layout is
// not fixed -- an encoder is free to write extra chunks (this project's
// source files carry a LIST/INFO chunk from ffmpeg) before the audio data, so
// the header is walked rather than assumed to be 44 bytes fixed like the
// BlocksDS example gets away with for its hand-made test file.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <maxmod9.h>
#include <nds.h>

#include "music.h"

#define STAGING_LEN 16384

static FILE *file;
static char current[32]; // bare name last passed to musicPlay(), "" if none

static uint32_t data_offset, data_size;
static uint16_t block_align;
static mm_stream_formats format;

static char staging[STAGING_LEN];
static int staging_in, staging_out;

// ---- WAV parsing ------------------------------------------------------------

static bool read_u32(FILE *f, uint32_t *out) {
  uint8_t b[4];
  if (fread(b, 1, 4, f) != 4)
    return false;
  *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
         ((uint32_t)b[3] << 24);
  return true;
}

static bool read_u16(FILE *f, uint16_t *out) {
  uint8_t b[2];
  if (fread(b, 1, 2, f) != 2)
    return false;
  *out = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
  return true;
}

static mm_stream_formats stream_format(uint16_t channels, uint16_t bits) {
  if (channels == 1)
    return bits == 8 ? MM_STREAM_8BIT_MONO : MM_STREAM_16BIT_MONO;
  return bits == 8 ? MM_STREAM_8BIT_STEREO : MM_STREAM_16BIT_STEREO;
}

// Walks RIFF chunks looking for "fmt " and "data". Leaves the file positioned
// at the start of the sample data on success.
static bool parse_wav(FILE *f, uint32_t *sample_rate) {
  char tag[4];
  uint32_t riff_size, sample_rate_field;
  uint16_t channels = 2, bits = 16;

  if (fread(tag, 1, 4, f) != 4 || memcmp(tag, "RIFF", 4) != 0)
    return false;
  if (!read_u32(f, &riff_size))
    return false;
  if (fread(tag, 1, 4, f) != 4 || memcmp(tag, "WAVE", 4) != 0)
    return false;

  bool have_fmt = false, have_data = false;

  while (!have_data) {
    uint32_t chunk_size;
    if (fread(tag, 1, 4, f) != 4 || !read_u32(f, &chunk_size))
      return false;

    long chunk_start = ftell(f);

    if (memcmp(tag, "fmt ", 4) == 0) {
      uint16_t audio_format;
      uint32_t byte_rate;

      if (!read_u16(f, &audio_format) || !read_u16(f, &channels) ||
          !read_u32(f, &sample_rate_field) || !read_u32(f, &byte_rate) ||
          !read_u16(f, &block_align) || !read_u16(f, &bits))
        return false;
      have_fmt = true;
    } else if (memcmp(tag, "data", 4) == 0) {
      data_offset = (uint32_t)chunk_start;
      data_size = chunk_size;
      have_data = true;
      break; // do not seek past the data we came for
    }

    // Chunks are padded to an even size.
    fseek(f, chunk_start + (long)chunk_size + (chunk_size & 1), SEEK_SET);
  }

  if (!have_fmt || !have_data)
    return false;

  format = stream_format(channels, bits);
  *sample_rate = sample_rate_field;
  return true;
}

// ---- Streaming ---------------------------------------------------------------

// Reads exactly `size` bytes into `buffer`, looping back to the start of the
// sample data when the file runs out.
static void read_loop(char *buffer, size_t size) {
  while (size > 0) {
    size_t got = fread(buffer, 1, size, file);
    size -= got;
    buffer += got;

    if (size > 0) {
      fseek(file, data_offset, SEEK_SET);
    }
  }
}

static void fill_staging(bool force) {
  if (!force && staging_in == staging_out)
    return;

  if (staging_in < staging_out) {
    size_t n = staging_out - staging_in;
    read_loop(&staging[staging_in], n);
    staging_in += n;
  } else {
    size_t n = STAGING_LEN - staging_in;
    read_loop(&staging[staging_in], n);
    staging_in = 0;

    n = staging_out - staging_in;
    read_loop(&staging[staging_in], n);
    staging_in += n;
  }

  if (staging_in >= STAGING_LEN)
    staging_in -= STAGING_LEN;
}

static mm_word stream_callback(mm_word length, mm_addr dest,
                               mm_stream_formats fmt) {
  size_t per_sample = (fmt == MM_STREAM_8BIT_MONO)    ? 1
                      : (fmt == MM_STREAM_8BIT_STEREO) ? 2
                      : (fmt == MM_STREAM_16BIT_MONO)  ? 2
                                                        : 4;
  size_t size = length * per_sample;
  size_t until_end = STAGING_LEN - staging_out;

  if (until_end > size) {
    memcpy(dest, &staging[staging_out], size);
    staging_out += size;
  } else {
    char *d = dest;
    memcpy(d, &staging[staging_out], until_end);
    d += until_end;
    size -= until_end;

    memcpy(d, &staging[0], size);
    staging_out = size;
  }

  return length;
}

// ---- Public interface --------------------------------------------------------

void musicInit(void) {
  mm_ds_system sys = {
      .mod_count = 0,
      .samp_count = 0,
      .mem_bank = 0,
      .fifo_channel = FIFO_MAXMOD,
  };
  mmInit(&sys);
  current[0] = '\0';
}

void musicPlay(const char *name) {
  if (strcmp(current, name) == 0)
    return;

  if (file != NULL) {
    mmStreamClose();
    fclose(file);
    file = NULL;
  }

  char path[48];
  snprintf(path, sizeof(path), "nitro:/audio/%s.wav", name);

  file = fopen(path, "rb");
  if (file == NULL)
    return;

  uint32_t sample_rate;
  if (!parse_wav(file, &sample_rate)) {
    fclose(file);
    file = NULL;
    return;
  }

  strncpy(current, name, sizeof(current) - 1);
  current[sizeof(current) - 1] = '\0';

  staging_in = 0;
  staging_out = 0;
  fill_staging(true);

  mm_stream stream = {
      .sampling_rate = sample_rate,
      .buffer_length = 2048,
      .callback = stream_callback,
      .format = format,
      .timer = MM_TIMER0,
      .manual = false,
  };
  mmStreamOpen(&stream);
}

void musicUpdate(void) {
  if (file == NULL)
    return;
  fill_staging(false);
}
