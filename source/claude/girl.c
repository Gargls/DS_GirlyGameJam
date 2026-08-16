// SPDX-License-Identifier: CC0-1.0
//
// The Popular Girl portrait.
//
// A DS sprite is at most 64x64, so the 128x128 portrait is four sprites in a
// 2x2 grid. Each one holds all three moods as animation frames, so changing
// mood is four NF_SpriteFrame() calls and no reloading.

#include <stdbool.h>

#include <nds.h>

#include <nf_lib.h>

#include "girl.h"

#define CELL 64
#define GRID 3
#define CELLS (GRID * GRID)

// The art is used at its native 192x192, so it fills the screen height. Centred
// horizontally: (256 - 192) / 2.
#define GIRL_X 32
#define GIRL_Y 0

// Drawn behind the text. On the DS a sprite beats a background at the same
// priority, so the console needs a *lower* number than this to sit in front --
// see bgSetPriority() in main.c. That is what lets the portrait be full screen
// without burying the text.
#define GIRL_PRIORITY 3

// Sprite RAM slots are a *global* pool shared by both screens -- the array is
// indexed by id alone, not by screen. The saw minigame holds graphics slots 0
// and 1 and palette slot 0, so the portrait starts above them. Reusing a slot
// that is already loaded makes NFLib halt the ROM.
#define GFX_RAM 4
#define PAL_RAM 1

// VRAM slots and sprite ids are per screen, so these can start at 0 even
// though the bottom screen also uses low numbers.
#define GFX_VRAM 0
#define PAL_VRAM 0
#define ID_FIRST 0

static GirlMood current = GIRL_IDLE;

void girlInit(void) {
  // Top-screen sprites live in VRAM_B, with their palettes in VRAM_F. The text
  // console is a background in VRAM_A, so the two do not collide. This only
  // ORs the sprite-enable bit into REG_DISPCNT; it does not touch the video
  // mode, so the console survives.
  NF_InitSpriteSys(0, 128);

  NF_LoadSpritePal("sprite/girl", PAL_RAM);
  NF_VramSpritePal(0, PAL_RAM, PAL_VRAM);

  for (int i = 0; i < CELLS; i++) {
    char path[32];
    snprintf(path, sizeof(path), "sprite/girl_q%d", i);

    // 64x64 is the size of one frame, not of the file. Each file holds three
    // of them stacked; NFLib divides to work out the frame count.
    NF_LoadSpriteGfx(path, GFX_RAM + i, CELL, CELL);

    // keepframes = false puts every frame in VRAM up front, so switching mood
    // is a pointer change rather than a DMA. Nine cells x three moods x 4 KB is
    // 108 KB of the 128 KB arena -- it fits, and nothing else uses top-screen
    // sprites.
    NF_VramSpriteGfx(0, GFX_RAM + i, GFX_VRAM + i, false);

    NF_CreateSprite(0, ID_FIRST + i, GFX_VRAM + i, PAL_VRAM,
                    GIRL_X + (i % GRID) * CELL, GIRL_Y + (i / GRID) * CELL);
    NF_SpriteLayer(0, ID_FIRST + i, GIRL_PRIORITY);
  }

  current = GIRL_IDLE;
}

void girlSetMood(GirlMood mood) {
  if (mood == current)
    return;
  current = mood;

  for (int i = 0; i < CELLS; i++)
    NF_SpriteFrame(0, ID_FIRST + i, mood);
}

void girlShow(bool visible) {
  for (int i = 0; i < CELLS; i++)
    NF_ShowSprite(0, ID_FIRST + i, visible);
}

void girlSetOffset(int dx, int dy) {
  // Negative X is fine here. The DS stores a sprite's X in nine bits and treats
  // it as wrapping, so -224 is written as 288 and the hardware draws the sprite
  // off the left edge exactly as intended. (Y is only eight bits and wraps at
  // 256, which is why the slide is horizontal only -- a negative Y would
  // reappear at the top of the screen instead of hiding.)
  for (int i = 0; i < CELLS; i++)
    NF_MoveSprite(0, ID_FIRST + i, GIRL_X + (i % GRID) * CELL + dx,
                  GIRL_Y + (i / GRID) * CELL + dy);
}
