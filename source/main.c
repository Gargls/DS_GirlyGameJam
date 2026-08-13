// SPDX-License-Identifier: CC0-1.0
//
// Sawmill Party.

#include <stdbool.h>
#include <stdio.h>

#include <filesystem.h>
#include <nds.h>

#include <nf_lib.h>

typedef enum { GS_MENU, GS_GAME } GameState;

static GameState state = GS_MENU;

static void menu_draw(void) {
  printf("Hi");
  printf(" \x1b[12;4HTouch lower Screen to Start");
}
static void game_draw(void) {
  printf("\x1b[2J");
  printf("\x1b[2;2HPut the Saws into the Spots");
}

#define SAW_COUNT 3

// The sprite is 16 wide and 32 tall, same as saw_1.img.
#define SAW_W 16
#define SAW_H 32
#define SNAP_DIST 12

static int saw_x[SAW_COUNT] = {20, 20, 20};
static int saw_y[SAW_COUNT] = {10, 70, 130};
static bool placed[SAW_COUNT] = {false, false, false};

static const int slot_x[SAW_COUNT] = {200, 200, 200};
static const int slot_y[SAW_COUNT] = {10, 70, 130};
static bool slot_taken[SAW_COUNT] = {false, false, false};

static int held = -1;
static int grab_dx, grab_dy;
static bool won = false;

static void check_win(void) {
  if (won)
    return;
  for (int i = 0; i < SAW_COUNT; i++)
    if (!placed[i])
      return;
  won = true;
  printf("\x1b[5;2Hyou win yayyyyyy");
}

static void game_update(void) {
  if (keysHeld() & KEY_TOUCH) {
    // Pen is down: pick up a saw, then drag whatever is held.
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

  } else {
    // Pen is up. If a saw was being dragged, this frame is the release.
    if (held >= 0) {
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
  }
}

static void game_start(void) {
  // File to RAM
  NF_LoadSpriteGfx("sprite/saw_1", 0, 16, 32);
  NF_LoadSpriteGfx("sprite/saw_2", 1, 16, 32);
  NF_LoadSpritePal("sprite/saws", 0);

  // RAM to VRAM
  NF_VramSpriteGfx(1, 0, 0, false);
  NF_VramSpriteGfx(1, 1, 1, false);
  NF_VramSpritePal(1, 0, 0);

  // Slots first (ids 3..5, drawn behind), then the saws (ids 0..2, on top).
  for (int i = 0; i < SAW_COUNT; i++)
    NF_CreateSprite(1, 3 + i, 1, 0, slot_x[i], slot_y[i]);
  for (int i = 0; i < SAW_COUNT; i++)
    NF_CreateSprite(1, i, 0, 0, saw_x[i], saw_y[i]);
}
int main(int argc, char **argv) {
  // Top screen: text console.
  videoSetMode(MODE_0_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  PrintConsole top;
  consoleInit(&top, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
  consoleSelect(&top);
  lcdMainOnTop();

  // NitroFS must be mounted before NFLib reads any file.
  if (!nitroFSInit(NULL)) {
    perror("nitroFSInit()");
    while (1)
      swiWaitForVBlank();
  }
  NF_SetRootFolder("NITROFS");

  // Bottom screen: sprites only. NF_Set2D() first, it clears the sprite
  // enable bit that NF_InitSpriteSys() sets.
  NF_Set2D(1, 0);
  NF_InitSpriteBuffers();
  NF_InitSpriteSys(1);
  BG_PALETTE_SUB[0] = RGB15(3, 4, 8);
  menu_draw();
  while (1) {
    scanKeys();

    if (state == GS_MENU) {
      if (keysDown() & KEY_TOUCH) {
        state = GS_GAME;
        game_draw();
        game_start();
      }
    }

    if (state == GS_GAME) {
      game_update();
    }
    NF_SpriteOamSet(1);
    swiWaitForVBlank();
    oamUpdate(&oamSub);
  }

  return 0;
}
