// SPDX-License-Identifier: CC0-1.0
//
// "Add the Bow!" -- part two of Build-a-Bear. One piece, one mark: drag the
// bow onto her head and let go close enough to snap it into place.
//
// The backdrop is bg/bear_eyes_bg -- the same bear, now with the eyes from
// the previous step already on -- generated alongside BOW_X/Y by
// tools/gen_buildbear.py. It owns bottom-screen BG layer 1 for the round,
// exactly like mg_eyes.c's bear_base_bg does.

#include <stdbool.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define BOW_W 64
#define BOW_H 32
#define SNAP_DIST 10

#define BG_LAYER 1
#define BG_NAME "bearbg"

#define SLOT_X 96
#define SLOT_Y 25

// Left of the bear (which spans x 79..177), clear of the timer bar at y=180.
#define RESET_X 8
#define RESET_Y 140

#define ID 0

static int bow_x, bow_y;
static bool placed;
static bool held;
static int grab_dx, grab_dy;

static bool bowUpdate(void) {
  if (placed)
    return true;

  if (keysHeld() & KEY_TOUCH) {
    touchPosition t;
    touchRead(&t);

    if (!held && (keysDown() & KEY_TOUCH) && t.px >= bow_x &&
        t.px < bow_x + BOW_W && t.py >= bow_y && t.py < bow_y + BOW_H) {
      held = true;
      grab_dx = t.px - bow_x;
      grab_dy = t.py - bow_y;
    }

    if (held) {
      bow_x = t.px - grab_dx;
      bow_y = t.py - grab_dy;
      NF_MoveSprite(1, ID, bow_x, bow_y);
    }

  } else if (held) {
    // The pen is up and the bow was being dragged, so this frame is the
    // release. Snap it if it landed near the mark.
    int dx = bow_x - SLOT_X;
    int dy = bow_y - SLOT_Y;
    if (dx < 0)
      dx = -dx;
    if (dy < 0)
      dy = -dy;

    if (dx <= SNAP_DIST && dy <= SNAP_DIST) {
      bow_x = SLOT_X;
      bow_y = SLOT_Y;
      placed = true;
    }

    NF_MoveSprite(1, ID, bow_x, bow_y);
    held = false;
  }

  return placed;
}

static void bowStart(void) {
  bow_x = RESET_X;
  bow_y = RESET_Y;
  placed = false;
  held = false;

  NF_LoadTiledBg("bg/bear_eyes_bg", BG_NAME, 256, 256);
  NF_CreateTiledBg(1, BG_LAYER, BG_NAME);

  NF_LoadSpriteGfx("sprite/bow", 0, BOW_W, BOW_H);
  NF_LoadSpritePal("sprite/bow", 0);
  NF_VramSpriteGfx(1, 0, 0, false);
  NF_VramSpritePal(1, 0, 0);

  // The mark first (id 1, drawn behind), then the draggable bow (id 0).
  NF_CreateSprite(1, 1, 0, 0, SLOT_X, SLOT_Y);
  NF_CreateSprite(1, ID, 0, 0, bow_x, bow_y);
}

static void bowStop(void) {
  NF_DeleteSprite(1, 0);
  NF_DeleteSprite(1, 1);

  NF_FreeSpriteGfx(1, 0);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(0);
  NF_UnloadSpritePal(0);

  NF_DeleteTiledBg(1, BG_LAYER);
  NF_UnloadTiledBg(BG_NAME);
}

const Minigame bowMinigame = {
    .name = "Add the Bow!",
    .hint = "Drag the bow onto the mark",
    .start = bowStart,
    .update = bowUpdate,
    .draw = NULL, // the hint never changes, so there is nothing to repaint
    .stop = bowStop,
};
