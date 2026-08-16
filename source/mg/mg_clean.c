// SPDX-License-Identifier: CC0-1.0
//
// Stage 3, game 3: "Clean it Up"
//
// Two phases in one game, because it is one job: scrub the blood off the floor,
// then drag what is left of him into the bag. The phase flips by itself the
// moment the last stain is gone, so the player is never told to switch -- they
// just run out of stains and the parts are what remain.
//
// Scrubbing counts stylus *travel* over a stain rather than taps, so wiping
// feels like wiping. (Contrast mg_cut.c, which counts direction changes because
// sawing is a different motion.)
//
// PLACEHOLDER ART -- ph_stain, ph_part and ph_bag from tools/gen_placeholders.py.
// The stain sheet is 32x128, four 32x32 frames that fade as they are scrubbed;
// the last frame is deliberately blank, so a cleaned stain simply renders as
// nothing and never has to be deleted mid-round.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define STAINS 4
#define PARTS 3

#define STAIN 32
#define STAIN_FRAMES 4
#define SCRUB_PER_FRAME 60 // pixels of travel to advance one frame

#define PART 16
#define BAG 32
#define BAG_X 208
#define BAG_Y 140

// Kept clear of the header rows (y < 32) and the timer bar (y = 180).
static const int stain_x[STAINS] = {24, 96, 60, 150};
static const int stain_y[STAINS] = {48, 96, 136, 52};

static const int part_home_x[PARTS] = {40, 110, 170};
static const int part_home_y[PARTS] = {100, 60, 120};

// Three sheets of three different sizes, so three graphics slots.
//
// Graphics and palette RAM slots are a GLOBAL pool across both screens
// (NF_SPR256GFX[] and NF_SPR256PAL[] have no screen index). Permanently taken:
// the girl holds graphics 4-12 and palette 1, the timer bar holds graphics 2
// and palette 2. So a minigame may use graphics 0, 1, 3 and palettes 0, 3, 4.
//
// The *extended palette* slots are a different pool and per screen
// (NF_SPR256VRAM[2][128]), so 0-2 are free here; the timer bar uses 3.
#define GFX_STAIN 0
#define GFX_PART 1
#define GFX_BAG 3
#define PAL_STAIN 0
#define PAL_PART 3
#define PAL_BAG 4
#define PAL_SLOT_STAIN 0
#define PAL_SLOT_PART 1
#define PAL_SLOT_BAG 2

#define ID_STAIN 0 // ids 0..STAINS-1
#define ID_PART 8  // ids 8..8+PARTS-1
#define ID_BAG 16

static int scrub[STAINS];  // accumulated travel per stain
static int part_x[PARTS], part_y[PARTS];
static bool bagged[PARTS];

static int held;      // which part is being dragged, -1 for none
static int grab_dx, grab_dy;
static int last_x, last_y;
static bool has_last;

static int stain_frame(int i) {
  int f = scrub[i] / SCRUB_PER_FRAME;
  return f > STAIN_FRAMES - 1 ? STAIN_FRAMES - 1 : f;
}

static bool stain_clean(int i) { return stain_frame(i) >= STAIN_FRAMES - 1; }

static bool all_clean(void) {
  for (int i = 0; i < STAINS; i++)
    if (!stain_clean(i))
      return false;
  return true;
}

static bool all_bagged(void) {
  for (int i = 0; i < PARTS; i++)
    if (!bagged[i])
      return false;
  return true;
}

// ---- phase 1: scrubbing -----------------------------------------------------

static void scrub_at(int x, int y, int travel) {
  for (int i = 0; i < STAINS; i++) {
    if (stain_clean(i))
      continue;
    if (x < stain_x[i] || x >= stain_x[i] + STAIN)
      continue;
    if (y < stain_y[i] || y >= stain_y[i] + STAIN)
      continue;

    int before = stain_frame(i);
    scrub[i] += travel;
    if (stain_frame(i) != before)
      NF_SpriteFrame(1, ID_STAIN + i, stain_frame(i));
    return; // one stylus position is over at most one stain
  }
}

// ---- phase 2: bagging -------------------------------------------------------

static bool over_bag(int x, int y) {
  return x >= BAG_X - PART && x < BAG_X + BAG && y >= BAG_Y - PART &&
         y < BAG_Y + BAG;
}

static void drag_parts(const touchPosition *t, bool pressed) {
  if (held < 0 && pressed) {
    for (int i = 0; i < PARTS; i++) {
      if (bagged[i])
        continue;
      if (t->px >= part_x[i] && t->px < part_x[i] + PART &&
          t->py >= part_y[i] && t->py < part_y[i] + PART) {
        held = i;
        grab_dx = t->px - part_x[i];
        grab_dy = t->py - part_y[i];
        break;
      }
    }
  }

  if (held < 0)
    return;

  part_x[held] = t->px - grab_dx;
  part_y[held] = t->py - grab_dy;
  NF_MoveSprite(1, held + ID_PART, part_x[held], part_y[held]);
}

// Called on the frame the stylus lifts.
static void release_part(void) {
  if (held < 0)
    return;

  if (over_bag(part_x[held] + PART / 2, part_y[held] + PART / 2)) {
    bagged[held] = true;
    NF_ShowSprite(1, held + ID_PART, false);
  }
  held = -1;
}

// ---- the game ---------------------------------------------------------------

static bool cleanUpdate(void) {
  bool down = (keysHeld() & KEY_TOUCH) != 0;

  if (!down) {
    release_part();
    has_last = false;
    return all_clean() && all_bagged();
  }

  touchPosition t;
  touchRead(&t);

  if (!all_clean()) {
    // Phase 1. Travel since the last frame is how much scrubbing happened.
    int travel = 0;
    if (has_last) {
      int dx = abs(t.px - last_x);
      int dy = abs(t.py - last_y);
      travel = dx + dy; // cheap distance; no square root needed for a threshold
    }
    scrub_at(t.px, t.py, travel);
  } else {
    // Phase 2.
    drag_parts(&t, (keysDown() & KEY_TOUCH) != 0);
  }

  last_x = t.px;
  last_y = t.py;
  has_last = true;

  return all_clean() && all_bagged();
}

static void cleanDraw(void) {
  if (!all_clean()) {
    int done = 0;
    for (int i = 0; i < STAINS; i++)
      if (stain_clean(i))
        done++;
    printf("\x1b[2;1HScrub the floor clean!        ");
    printf("\x1b[3;1H%d / %d stains gone  ", done, STAINS);
    return;
  }

  int done = 0;
  for (int i = 0; i < PARTS; i++)
    if (bagged[i])
      done++;
  printf("\x1b[2;1HNow bag the evidence!         ");
  printf("\x1b[3;1H%d / %d in the bag  ", done, PARTS);
}

static void cleanStart(void) {
  held = -1;
  has_last = false;

  for (int i = 0; i < STAINS; i++)
    scrub[i] = 0;
  for (int i = 0; i < PARTS; i++) {
    part_x[i] = part_home_x[i];
    part_y[i] = part_home_y[i];
    bagged[i] = false;
  }

  // 32x32 is one frame; the file is 32x128, so NFLib finds four.
  NF_LoadSpriteGfx("sprite/ph_stain", GFX_STAIN, STAIN, STAIN);
  NF_LoadSpritePal("sprite/ph_stain", PAL_STAIN);
  // 16x16 is one frame; the file is 16x32, so there are two part shapes.
  NF_LoadSpriteGfx("sprite/ph_part", GFX_PART, PART, PART);
  NF_LoadSpritePal("sprite/ph_part", PAL_PART);
  NF_LoadSpriteGfx("sprite/ph_bag", GFX_BAG, BAG, BAG);
  NF_LoadSpritePal("sprite/ph_bag", PAL_BAG);

  NF_VramSpriteGfx(1, GFX_STAIN, 0, false);
  NF_VramSpritePal(1, PAL_STAIN, PAL_SLOT_STAIN);
  NF_VramSpriteGfx(1, GFX_PART, 1, false);
  NF_VramSpritePal(1, PAL_PART, PAL_SLOT_PART);
  NF_VramSpriteGfx(1, GFX_BAG, 3, false);
  NF_VramSpritePal(1, PAL_BAG, PAL_SLOT_BAG);

  for (int i = 0; i < STAINS; i++) {
    NF_CreateSprite(1, ID_STAIN + i, 0, PAL_SLOT_STAIN, stain_x[i], stain_y[i]);
    NF_SpriteFrame(1, ID_STAIN + i, 0);
  }

  // Created before the parts, so a part being dragged passes over the bag
  // rather than disappearing behind it.
  NF_CreateSprite(1, ID_BAG, 3, PAL_SLOT_BAG, BAG_X, BAG_Y);

  for (int i = 0; i < PARTS; i++) {
    NF_CreateSprite(1, ID_PART + i, 1, PAL_SLOT_PART, part_x[i], part_y[i]);
    NF_SpriteFrame(1, ID_PART + i, 0);
  }

  cleanDraw();
}

// The exact mirror of start(), in reverse: sprites, then VRAM, then RAM.
static void cleanStop(void) {
  for (int i = 0; i < STAINS; i++)
    NF_DeleteSprite(1, ID_STAIN + i);
  for (int i = 0; i < PARTS; i++)
    NF_DeleteSprite(1, ID_PART + i);
  NF_DeleteSprite(1, ID_BAG);

  NF_FreeSpriteGfx(1, 0);
  NF_FreeSpriteGfx(1, 1);
  NF_FreeSpriteGfx(1, 3);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(GFX_STAIN);
  NF_UnloadSpriteGfx(GFX_PART);
  NF_UnloadSpriteGfx(GFX_BAG);
  NF_UnloadSpritePal(PAL_STAIN);
  NF_UnloadSpritePal(PAL_PART);
  NF_UnloadSpritePal(PAL_BAG);
}

const Minigame cleanMinigame = {
    .name = "Clean it Up",
    .hint = "Scrub the floor clean!",
    .start = cleanStart,
    .update = cleanUpdate,
    .draw = cleanDraw,
    .stop = cleanStop,
};
