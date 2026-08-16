// SPDX-License-Identifier: CC0-1.0
//
// "Set the Eyes!" -- part one of Build-a-Bear. Drag the two eye pieces onto
// the marks on the bear's face and let go close enough to snap them into
// place. Either piece may land in either socket; they are identical circles,
// so which one goes where was never going to matter to the player.
//
// The backdrop is the bear itself, not sprites: bg/bear_base_bg is a
// generated composite (see tools/gen_buildbear.py) that also fixes EYE_*_X/Y
// below -- re-run that tool and re-paste these if the source art changes.
// It owns bottom-screen BG layer 1 for the round, loaded and torn down here
// exactly like the sprites are, since nothing else needs that layer while
// this game is running.

#include <stdbool.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define EYE_COUNT 2
#define EYE_W 16
#define EYE_H 16
#define SNAP_DIST 10

#define BG_LAYER 1
#define BG_NAME "bearbg"

static const int slot_x[EYE_COUNT] = {103, 137};
static const int slot_y[EYE_COUNT] = {66, 66};

// Off to the side of the bear (which spans x 79..177), clear of the header
// and the timer bar at y=180.
static const int reset_x[EYE_COUNT] = {30, 30};
static const int reset_y[EYE_COUNT] = {70, 110};

static int eye_x[EYE_COUNT];
static int eye_y[EYE_COUNT];
static bool placed[EYE_COUNT];
static bool slot_taken[EYE_COUNT];

static int held;
static int grab_dx, grab_dy;
static bool won;

static void check_win(void) {
  if (won)
    return;
  for (int i = 0; i < EYE_COUNT; i++)
    if (!placed[i])
      return;
  won = true;
}

static bool eyesUpdate(void) {
  if (keysHeld() & KEY_TOUCH) {
    touchPosition t;
    touchRead(&t);

    if (held < 0 && (keysDown() & KEY_TOUCH)) {
      for (int i = 0; i < EYE_COUNT; i++) {
        if (placed[i])
          continue;
        if (t.px >= eye_x[i] && t.px < eye_x[i] + EYE_W &&
            t.py >= eye_y[i] && t.py < eye_y[i] + EYE_H) {
          held = i;
          grab_dx = t.px - eye_x[i];
          grab_dy = t.py - eye_y[i];
          break;
        }
      }
    }

    if (held >= 0) {
      eye_x[held] = t.px - grab_dx;
      eye_y[held] = t.py - grab_dy;
      NF_MoveSprite(1, held, eye_x[held], eye_y[held]);
    }

  } else if (held >= 0) {
    // The pen is up and something was being dragged, so this frame is the
    // release. Snap it if it landed near a free socket.
    for (int s = 0; s < EYE_COUNT; s++) {
      if (slot_taken[s])
        continue;

      int dx = eye_x[held] - slot_x[s];
      int dy = eye_y[held] - slot_y[s];
      if (dx < 0)
        dx = -dx;
      if (dy < 0)
        dy = -dy;

      if (dx <= SNAP_DIST && dy <= SNAP_DIST) {
        eye_x[held] = slot_x[s];
        eye_y[held] = slot_y[s];
        placed[held] = true;
        slot_taken[s] = true;
        break;
      }
    }

    NF_MoveSprite(1, held, eye_x[held], eye_y[held]);
    held = -1;
    check_win();
  }

  return won;
}

static void eyesStart(void) {
  for (int i = 0; i < EYE_COUNT; i++) {
    eye_x[i] = reset_x[i];
    eye_y[i] = reset_y[i];
    placed[i] = false;
    slot_taken[i] = false;
  }
  held = -1;
  won = false;

  NF_LoadTiledBg("bg/bear_base_bg", BG_NAME, 256, 256);
  NF_CreateTiledBg(1, BG_LAYER, BG_NAME);

  NF_LoadSpriteGfx("sprite/eyes", 0, EYE_W, EYE_H);
  NF_LoadSpritePal("sprite/eyes", 0);
  NF_VramSpriteGfx(1, 0, 0, false);
  NF_VramSpritePal(1, 0, 0);

  // Marks first (ids 2..3, drawn behind), then the draggable eyes (ids 0..1).
  for (int i = 0; i < EYE_COUNT; i++)
    NF_CreateSprite(1, EYE_COUNT + i, 0, 0, slot_x[i], slot_y[i]);
  for (int i = 0; i < EYE_COUNT; i++)
    NF_CreateSprite(1, i, 0, 0, eye_x[i], eye_y[i]);
}

// The exact mirror of start(), in reverse: sprites, then VRAM, then RAM, then
// the background.
static void eyesStop(void) {
  for (int i = 0; i < EYE_COUNT * 2; i++)
    NF_DeleteSprite(1, i);

  NF_FreeSpriteGfx(1, 0);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(0);
  NF_UnloadSpritePal(0);

  NF_DeleteTiledBg(1, BG_LAYER);
  NF_UnloadTiledBg(BG_NAME);
}

const Minigame eyesMinigame = {
    .name = "Set the Eyes!",
    .hint = "Drag the eyes onto the marks",
    .start = eyesStart,
    .update = eyesUpdate,
    .draw = NULL, // the hint never changes, so there is nothing to repaint
    .stop = eyesStop,
};
