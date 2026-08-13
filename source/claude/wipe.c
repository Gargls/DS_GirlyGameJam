// SPDX-License-Identifier: CC0-1.0
//
// The stripes wipe.
//
// A hardware window restricts a layer to a rectangle. Growing that rectangle
// from the left is the whole effect. WININ and WINOUT carry one bit per
// background layer plus a bit for sprites, so the window can hide the saws as
// well -- which is what lets the stripes cover the minigame without touching a
// single sprite priority.

#include <stdbool.h>

#include <nds.h>

#include <nf_lib.h>

#include "wipe.h"

// Frames for one sweep, and how fast the pattern crawls while it happens.
#define WIPE_FRAMES 20
#define WIPE_SCROLL 2

// Bit 4 of WININ/WINOUT is sprites. The backdrop colour is not gated by the
// window at all -- it shows wherever nothing else is drawn.
#define WIN_OBJ (1 << 4)

static int bg_layer = 3;
static bool running = false;
static bool covering = false;
static int frame = 0;
static int scroll = 0;

static inline u16 win_stripes(void) { return 1 << bg_layer; }

// Paints the window so columns [0, w) show one thing and the rest shows the
// other. Both directions use the same left-anchored rectangle growing 0 -> 256;
// only the inside/outside meanings swap. That is what makes the stripes clear
// away to the left rather than retreating back the way they came.
static void apply(int w) {
  u16 inside = covering ? win_stripes() : WIN_OBJ;
  u16 outside = covering ? WIN_OBJ : win_stripes();

  if (w <= 0) {
    // A zero-width window cannot be expressed: the hardware reads X0 == X1 as
    // undefined, not as empty. Drive both regions to the same value instead.
    REG_WININ_SUB = outside;
    REG_WINOUT_SUB = outside;
  } else if (w >= SCREEN_WIDTH) {
    // Nor can a full-width one -- the right edge is only 8 bits, so 256 does
    // not fit.
    REG_WININ_SUB = inside;
    REG_WINOUT_SUB = inside;
  } else {
    SUB_WIN0_X0 = 0;
    SUB_WIN0_X1 = (u8)w;
    SUB_WIN0_Y0 = 0;
    SUB_WIN0_Y1 = SCREEN_HEIGHT;
    REG_WININ_SUB = inside;
    REG_WINOUT_SUB = outside;
  }
}

void wipeInit(int layer) {
  bg_layer = layer;
  running = false;
}

void wipeStart(bool covering_) {
  covering = covering_;
  running = true;
  frame = 0;

  // The layer has to be on for the whole sweep either way; the window decides
  // where it is actually visible.
  NF_ShowBg(1, bg_layer);
  REG_DISPCNT_SUB |= DISPLAY_WIN0_ON;

  // Frame zero. Covering starts from a clear screen, clearing starts from a
  // full one -- apply() handles both from the same call.
  apply(0);
}

bool wipeIsRunning(void) { return running; }

void wipeUpdate(void) {
  if (!running)
    return;

  // The pattern is periodic in X, so scrolling the layer is the whole
  // animation.
  scroll = (scroll + WIPE_SCROLL) & 255;
  NF_ScrollBg(1, bg_layer, scroll, 0);

  frame++;
  apply((frame * SCREEN_WIDTH) / WIPE_FRAMES);

  if (frame < WIPE_FRAMES)
    return;

  running = false;
  REG_DISPCNT_SUB &= ~DISPLAY_WIN0_ON;

  // After a fill the stripes stay up as the cutscene backdrop. After a clear
  // they are done, so the layer goes off and the game has the screen.
  if (!covering)
    NF_HideBg(1, bg_layer);
}

void wipeCancel(void) {
  running = false;
  REG_DISPCNT_SUB &= ~DISPLAY_WIN0_ON;
  NF_HideBg(1, bg_layer);
}
