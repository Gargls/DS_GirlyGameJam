// SPDX-License-Identifier: CC0-1.0
//
// "Tap the Stars!" -- a discrimination game. Six spots each show a star or a
// bomb (two frames of one sprite sheet, flipped with NF_SpriteFrame like the
// girl's moods). Tap a star to score; tap a bomb and you lose a point. Reach
// STAR_TARGET before the clock runs out. The whole board reshuffles every so
// often to keep it moving.
//
// Bottom-screen slots: targets art on graphics slot 0 / extended palette 0,
// sprite ids 0..SPOTS-1 -- clear of the persistent timer bar (slot 2, ids 100+).

#include <stdbool.h>
#include <stdio.h>

#include <nds.h>

#include <nf_lib.h>

#include "games.h"

#define SPOTS 6
#define STAR_TARGET 8
#define SHUFFLE_FRAMES 50 // reshuffle the whole board this often
#define TAP_MARGIN 6

#define FRAME_STAR 0
#define FRAME_BOMB 1
#define SPR 16

// Spawn box: below the step header, above the timer bar at y=180.
#define MIN_X 8
#define MAX_X (256 - SPR - 8)
#define MIN_Y 26
#define MAX_Y 150

static int px[SPOTS], py[SPOTS];
static int kind[SPOTS]; // FRAME_STAR or FRAME_BOMB
static int score;
static int frames;
static unsigned seed;

static int rnd(int mod) {
  seed = seed * 1103515245u + 12345u;
  return (int)((seed >> 16) % (unsigned)mod);
}

// Drop spot i at a fresh position and type. Bomb roughly one time in three, so
// there is almost always a star to hit and the game stays winnable.
static void respawn(int i) {
  px[i] = MIN_X + rnd(MAX_X - MIN_X);
  py[i] = MIN_Y + rnd(MAX_Y - MIN_Y);
  kind[i] = (rnd(3) == 0) ? FRAME_BOMB : FRAME_STAR;
  NF_SpriteFrame(1, i, kind[i]);
  NF_MoveSprite(1, i, px[i], py[i]);
}

static bool starsUpdate(void) {
  if (++frames % SHUFFLE_FRAMES == 0)
    for (int i = 0; i < SPOTS; i++)
      respawn(i);

  if (keysDown() & KEY_TOUCH) {
    touchPosition t;
    touchRead(&t);
    for (int i = 0; i < SPOTS; i++) {
      if (t.px >= px[i] - TAP_MARGIN && t.px < px[i] + SPR + TAP_MARGIN &&
          t.py >= py[i] - TAP_MARGIN && t.py < py[i] + SPR + TAP_MARGIN) {
        if (kind[i] == FRAME_STAR)
          score++;
        else if (score > 0)
          score--; // tapped a bomb
        respawn(i);
        break; // one tap hits one spot
      }
    }
  }

  return score >= STAR_TARGET;
}

static void starsDraw(void) {
  printf("\x1b[3;1H%d / %d stars  ", score, STAR_TARGET);
}

static void starsStart(void) {
  score = 0;
  frames = 0;
  seed = 0x99;

  // 16x16 frames from the 16x32 sheet; keepframes=false puts both in VRAM so
  // flipping star<->bomb is a pointer change, not a reload.
  NF_LoadSpriteGfx("sprite/targets", 0, SPR, SPR);
  NF_LoadSpritePal("sprite/targets", 0);
  NF_VramSpriteGfx(1, 0, 0, false);
  NF_VramSpritePal(1, 0, 0);

  for (int i = 0; i < SPOTS; i++) {
    NF_CreateSprite(1, i, 0, 0, 0, 0);
    respawn(i);
  }

  starsDraw();
}

static void starsStop(void) {
  for (int i = 0; i < SPOTS; i++)
    NF_DeleteSprite(1, i);

  NF_FreeSpriteGfx(1, 0);
  NF_VramSpriteGfxDefrag(1);
  NF_UnloadSpriteGfx(0);
  NF_UnloadSpritePal(0);
}

const Minigame starsMinigame = {
    .name = "Tap the Stars!",
    .hint = "Tap stars, dodge the bombs!",
    .start = starsStart,
    .update = starsUpdate,
    .draw = starsDraw,
    .stop = starsStop,
};
