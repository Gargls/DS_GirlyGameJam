// SPDX-License-Identifier: CC0-1.0
//
// The girl's two one-shot reaction sounds, played from the mmutil soundbank
// (sfx/*.wav -> nitro:/soundbank.bin) rather than streamed -- these are
// fractions of a second, not multi-minute songs, so baking them into RAM
// whole is exactly what a soundbank is for. See music.c for the mmInit() and
// mmSoundBankInFiles() call this depends on.

#include <maxmod9.h>

#include "sfx.h"
#include "soundbank.h"

void sfxInit(void) {
  mmLoadEffect(SFX_HEHEHE);
  mmLoadEffect(SFX_HUH);
}

void sfxHappy(void) { mmEffect(SFX_HEHEHE); }

void sfxDisappointed(void) { mmEffect(SFX_HUH); }
