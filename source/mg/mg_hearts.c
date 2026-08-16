// SPDX-License-Identifier: CC0-1.0
//
// "Catch the Hearts!" -- a falling-objects minigame. Hearts drop from the top;
// drag the purse left/right along the bottom to catch them. Catch CATCH_TARGET
// before the round ends to win. Missed hearts just respawn, no penalty -- the
// clock is the pressure.
//
// Bottom-screen sprite slots (see timerbar.c for the whole map): heart art on
// graphics slot 0 / extended palette 0, purse on graphics slot 1 / extended
// palette 2. Sprite id 0 is the purse, ids 1..MAX_HEARTS are the hearts.

#include <stdbool.h>
#include <stdio.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define MAX_HEARTS 4
#define CATCH_TARGET 6

#define SCREEN_W 256
#define SCREEN_H 192

#define HEART 16
#define TRAY_W 32
#define TRAY_H 16
#define TRAY_Y 162 // just above the timer bar at y=180
#define FALL_SPEED 2

static int hx[MAX_HEARTS], hy[MAX_HEARTS];
static int tray_x;
static int catches;
static unsigned seed;

// A tiny linear-congruential RNG. Good enough to scatter spawn columns; keeps
// the game deterministic from a fixed seed and needs no srand().
static int rnd(int mod) {
  seed = seed * 1103515245u + 12345u;
  return (int)((seed >> 16) % (unsigned)mod);
}

// Send heart i back to the top at a fresh column.
static void spawn(int i) {
  hx[i] = rnd(SCREEN_W - HEART);
  hy[i] = -HEART - rnd(40);
}

static bool heartsUpdate(void) {
  // The purse tracks the stylus while it is down.
  if (keysHeld() & KEY_TOUCH) {
    touchPosition t;
    touchRead(&t);
    tray_x = t.px - TRAY_W / 2;
    if (tray_x < 0)
      tray_x = 0;
    if (tray_x > SCREEN_W - TRAY_W)
      tray_x = SCREEN_W - TRAY_W;
    NF_MoveSprite(1, 0, tray_x, TRAY_Y);
  }

  for (int i = 0; i < MAX_HEARTS; i++) {
    hy[i] += FALL_SPEED;

    bool gone = false;
    // Overlapping the purse's row? Catch if the heart's centre is over it.
    if (hy[i] + HEART >= TRAY_Y && hy[i] <= TRAY_Y + TRAY_H) {
      int cx = hx[i] + HEART / 2;
      if (cx >= tray_x && cx < tray_x + TRAY_W) {
        catches++;
        gone = true;
      }
    }
    if (!gone && hy[i] > SCREEN_H)
      gone = true; // fell past the bottom

    if (gone)
      spawn(i);
    NF_MoveSprite(1, 1 + i, hx[i], hy[i]);
  }

  return catches >= CATCH_TARGET;
}

static void heartsDraw(void) {
  printf("\x1b[3;1H%d / %d caught  ", catches, CATCH_TARGET);
}

static void heartsStart(void) {
  tray_x = (SCREEN_W - TRAY_W) / 2;
  catches = 0;
  seed = 0x1234;

  NF_LoadSpriteGfx("sprite/heart", 0, HEART, HEART);
  NF_LoadSpritePal("sprite/heart", 0);
  NF_LoadSpriteGfx("sprite/tray", 1, TRAY_W, TRAY_H);
  NF_LoadSpritePal("sprite/tray", 3);

  NF_VramSpriteGfx(1, 0, 0, false);
  NF_VramSpritePal(1, 0, 0);
  NF_VramSpriteGfx(1, 1, 1, false);
  NF_VramSpritePal(1, 3, 2);

  NF_CreateSprite(1, 0, 1, 2, tray_x, TRAY_Y); // purse
  for (int i = 0; i < MAX_HEARTS; i++) {
    spawn(i);
    hy[i] = -HEART - i * 45; // stagger the first drop so they don't clump
    NF_CreateSprite(1, 1 + i, 0, 0, hx[i], hy[i]);
  }

  heartsDraw();
}

static void heartsStop(void) {
  for (int i = 0; i <= MAX_HEARTS; i++)
    NF_DeleteSprite(1, i);

  NF_FreeSpriteGfx(1, 0);
  NF_FreeSpriteGfx(1, 1);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(0);
  NF_UnloadSpriteGfx(1);
  NF_UnloadSpritePal(0);
  NF_UnloadSpritePal(3);
}

const Minigame heartsMinigame = {
    .name = "Catch the Hearts!",
    .hint = "Drag the purse to catch them",
    .start = heartsStart,
    .update = heartsUpdate,
    .draw = heartsDraw,
    .stop = heartsStop,
};
