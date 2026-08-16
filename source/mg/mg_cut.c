// SPDX-License-Identifier: CC0-1.0
//
// Stage 3, game 2: "Cut up the Body"
//
// Three cut lines are drawn across the body. Drag the stylus back and forth
// across a line to saw through it; each change of direction while you are on
// the line counts as one stroke. Sever all three before the clock runs out.
//
// Counting *direction changes* rather than distance is what makes it feel like
// sawing. Distance alone would let one long slow drag finish a cut, and holding
// still would do nothing at all -- neither reads as the motion we want.
//
// PLACEHOLDER ART -- ph_body and ph_cut from tools/gen_placeholders.py. The cut
// marks are a 32x32 sheet holding four 32x8 frames (untouched, a third, two
// thirds, severed), so advancing a cut is one NF_SpriteFrame call.

#include <stdbool.h>
#include <stdio.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define CUTS 3
#define STROKES_PER_CUT 6 // strokes to go from untouched to severed
#define FRAMES 4          // frames in the ph_cut sheet

#define BODY_W 64
#define BODY_H 64
#define BODY_X ((256 - BODY_W) / 2)
#define BODY_Y 56

#define MARK_W 32
#define MARK_H 8

// How far off a line the stylus may stray and still be sawing it.
#define BAND 10

// Palette RAM slot 1 is NOT available here: it belongs to the girl, and that
// pool is global across both screens (NF_SPR256PAL[] has no screen index).
// Slot 2 is the timer bar's. Minigames get 0 and 3 upwards.
//
// The *extended palette* slots below are a different thing and are per screen
// (NF_SPR256VRAM[2][128]), so 0 and 1 are free here even though the girl uses
// extended slot 0 on the top screen.
#define GFX_BODY 0
#define GFX_MARK 1
#define PAL_BODY 0
#define PAL_MARK 3
#define PAL_SLOT_BODY 0
#define PAL_SLOT_MARK 1

#define ID_BODY 0
#define ID_MARK 1 // ids 1..CUTS

// Cut lines across the arms and the legs, in screen coordinates.
static const int mark_x[CUTS] = {BODY_X + 4, BODY_X + 28, BODY_X + 16};
static const int mark_y[CUTS] = {BODY_Y + 22, BODY_Y + 22, BODY_Y + 48};

static int strokes[CUTS];
static int active;    // which cut the stylus is on, -1 for none
static int last_x;    // previous stylus x, for detecting a direction change
static int direction; // -1, 0 or +1

static int cut_stage(int i) {
  // Map strokes onto the four art frames: 0, then thirds of the way through.
  int s = (strokes[i] * (FRAMES - 1)) / STROKES_PER_CUT;
  return s > FRAMES - 1 ? FRAMES - 1 : s;
}

static bool all_cut(void) {
  for (int i = 0; i < CUTS; i++)
    if (strokes[i] < STROKES_PER_CUT)
      return false;
  return true;
}

// Which cut line is the stylus over, if any?
static int line_at(int x, int y) {
  for (int i = 0; i < CUTS; i++) {
    if (strokes[i] >= STROKES_PER_CUT)
      continue; // already severed
    if (x >= mark_x[i] - BAND && x < mark_x[i] + MARK_W + BAND &&
        y >= mark_y[i] - BAND && y < mark_y[i] + MARK_H + BAND)
      return i;
  }
  return -1;
}

static bool cutUpdate(void) {
  if (!(keysHeld() & KEY_TOUCH)) {
    // Pen up ends the current stroke, so lifting off and coming back does not
    // register as a change of direction.
    active = -1;
    direction = 0;
    return all_cut();
  }

  touchPosition t;
  touchRead(&t);

  int line = line_at(t.px, t.py);

  if (line != active) {
    // Arrived on a new line (or wandered off one). Start a fresh stroke.
    active = line;
    direction = 0;
    last_x = t.px;
    return all_cut();
  }

  if (active < 0)
    return all_cut();

  int dx = t.px - last_x;
  if (dx > 2 || dx < -2) {
    int dir = dx > 0 ? 1 : -1;
    if (direction != 0 && dir != direction) {
      // A change of direction while on the line: one stroke of the saw.
      if (strokes[active] < STROKES_PER_CUT) {
        strokes[active]++;
        NF_SpriteFrame(1, ID_MARK + active, cut_stage(active));
      }
    }
    direction = dir;
    last_x = t.px;
  }

  return all_cut();
}

static void cutDraw(void) {
  int done = 0;
  for (int i = 0; i < CUTS; i++)
    if (strokes[i] >= STROKES_PER_CUT)
      done++;
  printf("\x1b[3;1H%d / %d cut through  ", done, CUTS);
}

static void cutStart(void) {
  for (int i = 0; i < CUTS; i++)
    strokes[i] = 0;
  active = -1;
  direction = 0;
  last_x = 0;

  NF_LoadSpriteGfx("sprite/ph_body", GFX_BODY, BODY_W, BODY_H);
  NF_LoadSpritePal("sprite/ph_body", PAL_BODY);
  // 32x8 is one frame; the file is 32x32, so NFLib finds four of them.
  NF_LoadSpriteGfx("sprite/ph_cut", GFX_MARK, MARK_W, MARK_H);
  NF_LoadSpritePal("sprite/ph_cut", PAL_MARK);

  NF_VramSpriteGfx(1, GFX_BODY, 0, false);
  NF_VramSpritePal(1, PAL_BODY, PAL_SLOT_BODY);
  NF_VramSpriteGfx(1, GFX_MARK, 1, false);
  NF_VramSpritePal(1, PAL_MARK, PAL_SLOT_MARK);

  // Body first so the marks, created after, draw on top of it.
  NF_CreateSprite(1, ID_BODY, 0, PAL_SLOT_BODY, BODY_X, BODY_Y);
  for (int i = 0; i < CUTS; i++) {
    NF_CreateSprite(1, ID_MARK + i, 1, PAL_SLOT_MARK, mark_x[i], mark_y[i]);
    NF_SpriteFrame(1, ID_MARK + i, 0);
  }

  cutDraw();
}

// The exact mirror of start(), in reverse: sprites, then VRAM, then RAM.
static void cutStop(void) {
  for (int i = 0; i <= CUTS; i++)
    NF_DeleteSprite(1, ID_BODY + i);

  NF_FreeSpriteGfx(1, 0);
  NF_FreeSpriteGfx(1, 1);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(GFX_BODY);
  NF_UnloadSpriteGfx(GFX_MARK);
  NF_UnloadSpritePal(PAL_BODY);
  NF_UnloadSpritePal(PAL_MARK);
}

const Minigame cutMinigame = {
    .name = "Cut up the Body",
    .hint = "Saw back and forth on the lines",
    .start = cutStart,
    .update = cutUpdate,
    .draw = cutDraw,
    .stop = cutStop,
};
