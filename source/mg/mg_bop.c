// SPDX-License-Identifier: CC0-1.0
//
// Stage 3, game 1: "Bop the Head!"
//
// Whack-a-mole. A head appears at one of six spots; tap it before it moves on.
// Land TARGET hits before the clock runs out.
//
// PLACEHOLDER ART -- ph_head, from tools/gen_placeholders.py. The sheet is
// 32x64, which is two 32x32 frames: frame 0 is the head up, frame 1 is the
// bopped reaction. Swapping between them is NF_SpriteFrame(), the same trick
// the girl uses for her moods, so a hit costs nothing but a pointer change.

#include <stdbool.h>
#include <stdio.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define SPOTS 6
#define TARGET 6
#define HEAD 32

#define FRAME_UP 0
#define FRAME_BOPPED 1

// Frames the bopped face stays up before the head moves on, and how long a head
// sits in one place before hopping by itself.
#define BOP_FRAMES 12
#define HOP_FRAMES 45

#define TAP_MARGIN 6

#define GFX_RAM 0
#define PAL_RAM 0
#define PAL_SLOT 0
#define ID 0

// Two rows of three, clear of the header rows at the top (y < 32) and the timer
// bar at y=180.
static const int spot_x[SPOTS] = {16, 112, 208, 16, 112, 208};
static const int spot_y[SPOTS] = {44, 44, 44, 116, 116, 116};

static int hits;
static int spot;
static int timer;   // frames until the head hops on its own
static int bopped;  // frames left showing the bopped face, 0 when up

static void move_to(int s) {
  spot = s;
  timer = HOP_FRAMES;
  bopped = 0;
  NF_SpriteFrame(1, ID, FRAME_UP);
  NF_MoveSprite(1, ID, spot_x[spot], spot_y[spot]);
}

// Step by 5 across 6 spots. 5 and 6 share no factors, so the head visits every
// spot before repeating and never lands twice in a row -- cheaper and more
// predictable than a random number generator, and it can never sit still.
static void hop(void) { move_to((spot + 5) % SPOTS); }

static bool bopUpdate(void) {
  if (bopped > 0) {
    // Holding the reaction. Input is ignored so one tap cannot count twice.
    if (--bopped == 0)
      hop();
    return hits >= TARGET;
  }

  if (--timer <= 0)
    hop();

  if (keysDown() & KEY_TOUCH) {
    touchPosition t;
    touchRead(&t);
    if (t.px >= spot_x[spot] - TAP_MARGIN &&
        t.px < spot_x[spot] + HEAD + TAP_MARGIN &&
        t.py >= spot_y[spot] - TAP_MARGIN &&
        t.py < spot_y[spot] + HEAD + TAP_MARGIN) {
      hits++;
      bopped = BOP_FRAMES;
      NF_SpriteFrame(1, ID, FRAME_BOPPED);
    }
  }

  return hits >= TARGET;
}

static void bopDraw(void) { printf("\x1b[3;1H%d / %d bopped  ", hits, TARGET); }

static void bopStart(void) {
  hits = 0;

  // 32x32 is the size of one frame, not of the file -- the file is 32x64 and
  // NFLib divides to find the two frames. keepframes = false pushes both into
  // VRAM up front so NF_SpriteFrame never has to touch the card.
  NF_LoadSpriteGfx("sprite/ph_head", GFX_RAM, HEAD, HEAD);
  NF_LoadSpritePal("sprite/ph_head", PAL_RAM);
  NF_VramSpriteGfx(1, GFX_RAM, 0, false);
  NF_VramSpritePal(1, PAL_RAM, PAL_SLOT);

  NF_CreateSprite(1, ID, 0, PAL_SLOT, spot_x[0], spot_y[0]);
  move_to(0);

  bopDraw();
}

// The exact mirror of start(), in reverse: sprite, then VRAM, then RAM.
static void bopStop(void) {
  NF_DeleteSprite(1, ID);

  NF_FreeSpriteGfx(1, 0);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(GFX_RAM);
  NF_UnloadSpritePal(PAL_RAM);
}

const Minigame bopMinigame = {
    .name = "Bop the Head!",
    .hint = "Tap it before it moves!",
    .start = bopStart,
    .update = bopUpdate,
    .draw = bopDraw,
    .stop = bopStop,
};
