// SPDX-License-Identifier: CC0-1.0
//
// The stand-in. Used everywhere a real minigame is still waiting on art --
// every game in stage 1 and stage 4.
//
// It is deliberately playable rather than an empty screen that auto-wins: the
// point of a placeholder here is to prove the surrounding machinery works, and
// a game you can lose is the only way to exercise the clock, the life count and
// the fail path. Tap the target TARGET times before the round ends.
//
// It also uses no art at all. It borrows the timer-bar segment already in VRAM,
// which means this file can never be the reason a build breaks on a missing
// asset -- exactly what you want from a placeholder.

#include <stdbool.h>
#include <stdio.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define TARGET 5
#define SPR 8    // the bar segment is 8x8
#define BOX 28   // generous tap area around it
#define SPOTS 6

// Kept clear of the header rows at the top and the timer bar at y=180.
static const int spot_x[SPOTS] = {40, 128, 208, 60, 150, 100};
static const int spot_y[SPOTS] = {50, 40, 70, 120, 130, 90};

#define GFX_RAM 0
#define PAL_RAM 0
#define PAL_SLOT 0
#define ID 0

static int hits;
static int spot;

// Step by 5 across 6 spots. 5 and 6 share no factors, so it visits all six
// before repeating and can never land on the same one twice running -- cheaper
// and more predictable than a random number generator.
static void hop(void) {
  spot = (spot + 5) % SPOTS;
  NF_MoveSprite(1, ID, spot_x[spot], spot_y[spot]);
}

static bool placeholderUpdate(void) {
  if (keysDown() & KEY_TOUCH) {
    touchPosition t;
    touchRead(&t);

    int cx = spot_x[spot] + SPR / 2;
    int cy = spot_y[spot] + SPR / 2;
    if (t.px >= cx - BOX / 2 && t.px < cx + BOX / 2 && t.py >= cy - BOX / 2 &&
        t.py < cy + BOX / 2) {
      hits++;
      hop();
    }
  }
  return hits >= TARGET;
}

static void placeholderDraw(void) {
  printf("\x1b[3;1HPLACEHOLDER  %d / %d   ", hits, TARGET);
}

static void placeholderStart(void) {
  hits = 0;
  spot = 0;

  NF_LoadSpriteGfx("sprite/bar", GFX_RAM, SPR, SPR);
  NF_LoadSpritePal("sprite/bar", PAL_RAM);
  NF_VramSpriteGfx(1, GFX_RAM, 0, false);
  NF_VramSpritePal(1, PAL_RAM, PAL_SLOT);

  NF_CreateSprite(1, ID, 0, PAL_SLOT, spot_x[spot], spot_y[spot]);
  placeholderDraw();
}

// The exact mirror of start(), in reverse: sprite, then VRAM, then RAM.
static void placeholderStop(void) {
  NF_DeleteSprite(1, ID);

  NF_FreeSpriteGfx(1, 0);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(GFX_RAM);
  NF_UnloadSpritePal(PAL_RAM);
}

const Minigame placeholderMinigame = {
    .name = "[PLACEHOLDER GAME]",
    .hint = "Tap the block!",
    .start = placeholderStart,
    .update = placeholderUpdate,
    .draw = placeholderDraw,
    .stop = placeholderStop,
};
