// SPDX-License-Identifier: CC0-1.0
//
// The timer bar. A row of PIPS solid 8x8 segments; the fuller the bar, the more
// time is left. Segments are hidden from the right as the clock drains, so it
// empties toward the left like a WarioWare timer.
//
// Why segments and not one scaled sprite: a DS sprite is at most 64 px wide, so
// a 128 px bar needs several sprites anyway, and hardware affine scaling shrinks
// around a sprite's centre, which fights a left-anchored bar. Sixteen on/off
// pips are simpler and cannot render garbage.
//
// It lives on the bottom screen (1) permanently, on sprite/palette slots no
// minigame touches, so loading a game never disturbs it:
//   graphics RAM/VRAM slot 2, palette RAM slot 2, extended palette slot 3,
//   sprite ids 100..115. (Saw/whack/hearts/stars use gfx 0-1, pal ext 0-2;
//   the girl portrait owns palette RAM slot 1 on the top screen.)

#include <nds.h>

#include <nf_lib.h>

#include "timerbar.h"

#define PIPS 16
#define PIP 8                    // 8x8 segment
#define BAR_W (PIPS * PIP)       // 128 px
#define BAR_X ((256 - BAR_W) / 2) // centred
#define BAR_Y 180                // near the bottom, clear of the play area

#define GFX_RAM 2
#define GFX_VRAM 2
#define PAL_RAM 2
#define PAL_SLOT 3
#define ID_FIRST 100

static int shown = -1; // how many pips are currently visible; -1 = uninitialised

void timerBarInit(void) {
  NF_LoadSpriteGfx("sprite/bar", GFX_RAM, PIP, PIP);
  NF_LoadSpritePal("sprite/bar", PAL_RAM);
  NF_VramSpriteGfx(1, GFX_RAM, GFX_VRAM, true);
  NF_VramSpritePal(1, PAL_RAM, PAL_SLOT);

  for (int i = 0; i < PIPS; i++) {
    NF_CreateSprite(1, ID_FIRST + i, GFX_VRAM, PAL_SLOT, BAR_X + i * PIP, BAR_Y);
    NF_ShowSprite(1, ID_FIRST + i, false);
  }
  shown = 0;
}

void timerBarSet(int left, int total) {
  if (total <= 0)
    return;
  if (left < 0)
    left = 0;

  // Round up so any time left keeps at least one pip lit, and the bar only
  // empties completely when the clock genuinely hits zero.
  int lit = (left * PIPS + total - 1) / total;
  if (lit > PIPS)
    lit = PIPS;

  if (lit == shown)
    return; // nothing changed this frame
  for (int i = 0; i < PIPS; i++)
    NF_ShowSprite(1, ID_FIRST + i, i < lit);
  shown = lit;
}

void timerBarHide(void) {
  if (shown == 0)
    return;
  for (int i = 0; i < PIPS; i++)
    NF_ShowSprite(1, ID_FIRST + i, false);
  shown = 0;
}
