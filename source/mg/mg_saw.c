// SPDX-License-Identifier: CC0-1.0
//
// The drag puzzle that used to be inline in main.c: drag three saws onto three
// marks, snapping when they are close enough. Same code, now behind the
// Minigame interface with everything static, so the engine only ever sees the
// handle at the bottom.

#include <stdbool.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define SAW_COUNT 3
#define SAW_W 16
#define SAW_H 32
#define SNAP_DIST 12

static int saw_x[SAW_COUNT];
static int saw_y[SAW_COUNT];
static bool placed[SAW_COUNT];

static const int saw_reset_x[SAW_COUNT] = {20, 20, 20};
static const int saw_reset_y[SAW_COUNT] = {40, 90, 140};

static const int slot_x[SAW_COUNT] = {200, 200, 200};
static const int slot_y[SAW_COUNT] = {40, 90, 140};
static bool slot_taken[SAW_COUNT];

static int held;
static int grab_dx, grab_dy;
static bool won;

static void check_win(void) {
  if (won)
    return;
  for (int i = 0; i < SAW_COUNT; i++)
    if (!placed[i])
      return;
  won = true;
}

static bool sawUpdate(void) {
  if (keysHeld() & KEY_TOUCH) {
    touchPosition t;
    touchRead(&t);

    if (held < 0 && (keysDown() & KEY_TOUCH)) {
      for (int i = 0; i < SAW_COUNT; i++) {
        if (placed[i])
          continue;
        if (t.px >= saw_x[i] && t.px < saw_x[i] + SAW_W && t.py >= saw_y[i] &&
            t.py < saw_y[i] + SAW_H) {
          held = i;
          grab_dx = t.px - saw_x[i];
          grab_dy = t.py - saw_y[i];
          break;
        }
      }
    }

    if (held >= 0) {
      saw_x[held] = t.px - grab_dx;
      saw_y[held] = t.py - grab_dy;
      NF_MoveSprite(1, held, saw_x[held], saw_y[held]);
    }

  } else if (held >= 0) {
    // The pen is up and something was being dragged, so this frame is the
    // release. Snap it if it landed near a free slot.
    for (int s = 0; s < SAW_COUNT; s++) {
      if (slot_taken[s])
        continue;

      int dx = saw_x[held] - slot_x[s];
      int dy = saw_y[held] - slot_y[s];
      if (dx < 0)
        dx = -dx;
      if (dy < 0)
        dy = -dy;

      if (dx <= SNAP_DIST && dy <= SNAP_DIST) {
        saw_x[held] = slot_x[s];
        saw_y[held] = slot_y[s];
        placed[held] = true;
        slot_taken[s] = true;
        break;
      }
    }

    NF_MoveSprite(1, held, saw_x[held], saw_y[held]);
    held = -1;
    check_win();
  }

  return won;
}

static void sawStart(void) {
  for (int i = 0; i < SAW_COUNT; i++) {
    saw_x[i] = saw_reset_x[i];
    saw_y[i] = saw_reset_y[i];
    placed[i] = false;
    slot_taken[i] = false;
  }
  held = -1;
  won = false;

  NF_LoadSpriteGfx("sprite/saw_1", 0, SAW_W, SAW_H);
  NF_LoadSpriteGfx("sprite/saw_2", 1, SAW_W, SAW_H);
  NF_LoadSpritePal("sprite/saws", 0);

  NF_VramSpriteGfx(1, 0, 0, false);
  NF_VramSpriteGfx(1, 1, 1, false);
  NF_VramSpritePal(1, 0, 0);

  // Marks first (ids 3..5, drawn behind), then the saws (ids 0..2, on top).
  for (int i = 0; i < SAW_COUNT; i++)
    NF_CreateSprite(1, 3 + i, 1, 0, slot_x[i], slot_y[i]);
  for (int i = 0; i < SAW_COUNT; i++)
    NF_CreateSprite(1, i, 0, 0, saw_x[i], saw_y[i]);
}

// The exact mirror of start(), in reverse order: sprites, then VRAM, then RAM.
static void sawStop(void) {
  for (int i = 0; i < SAW_COUNT * 2; i++)
    NF_DeleteSprite(1, i);

  NF_FreeSpriteGfx(1, 0);
  NF_FreeSpriteGfx(1, 1);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(0);
  NF_UnloadSpriteGfx(1);
  NF_UnloadSpritePal(0);
}

const Minigame sawMinigame = {
    .name = "Put them in place!",
    .hint = "Drag the saws onto the marks",
    .start = sawStart,
    .update = sawUpdate,
    .draw = NULL, // the hint never changes, so there is nothing to repaint
    .stop = sawStop,
};
