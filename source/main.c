// SPDX-License-Identifier: CC0-1.0

#include <stdbool.h>
#include <stdio.h>

#include <filesystem.h>
#include <nds.h>

#include <nf_lib.h>

// Written by Claude -- see source/claude/
#include "claude/girl.h"
#include "claude/round.h"
#include "claude/topbg.h"
#include "claude/wipe.h"

// Seconds on the clock for a minigame step.
#define ROUND_SECONDS 5

// Declared up here so the campaign code can call into the minigame code even
// though the minigame is defined further down the file.
static void step_advance(void);
static void game_start(void);
static void game_stop(void);

typedef enum { GS_MENU, GS_STEP } GameState;

static GameState state = GS_MENU;

// The stripes background. Layer 3 is the backmost of the four, which is what
// you want for a backdrop, and it leaves 0-2 free for the wipe later.
#define BG_STRIPES_LAYER 3

// Drop the Saw MiniGame Stuff -> TODO: Put it into its own file
#define SAW_COUNT 3
#define SAW_W 16
#define SAW_H 32
#define SNAP_DIST 12

static int saw_x[SAW_COUNT] = {20, 20, 20};
static int saw_y[SAW_COUNT] = {10, 70, 130};
static bool placed[SAW_COUNT] = {false, false, false};
// Reset
static const int saw_reset_x[SAW_COUNT] = {20, 20, 20};
static const int saw_reset_y[SAW_COUNT] = {10, 70, 130};

static const int slot_x[SAW_COUNT] = {200, 200, 200};
static const int slot_y[SAW_COUNT] = {10, 70, 130};
static bool slot_taken[SAW_COUNT] = {false, false, false};

static int held = -1;
static int grab_dx, grab_dy;
static bool won = false;
// Until here

// Campaign Module

static int step = 0;

typedef enum { STEP_CUTSCENE, STEP_MINIGAME } StepKind;

typedef struct {
  StepKind kind;
  const char *name;
} CampaignStep;

static const CampaignStep campaign[] = {
    {STEP_CUTSCENE, "Start the Day"},
    {STEP_MINIGAME, "Put the Saws into the Spot"},
    {STEP_CUTSCENE, "Day one done!"},
};
#define CAMPAIGN_LEN ((int)(sizeof(campaign) / sizeof(campaign[0])))

// Draws only. No input, no state changes
//
static void step_draw(void) {

  const CampaignStep *s = &campaign[step];

  printf("\x1b[2J");
  printf("\x1b[1;2H%s  Step %d of %d",
         s->kind == STEP_MINIGAME ? "MINIGAME" : "CUTSCENE", step + 1,
         CAMPAIGN_LEN);
  printf("\x1b[2;2H%s", s->name);
  if (s->kind == STEP_CUTSCENE)
    printf("\x1b[21;2HTouch to continue");
}

static void menu_draw(void) {
  printf("\x1b[2J");
  printf("\x1b[1;2HGame");
  printf("\x1b[21;2HTouch lower Screen to Start");
}

// Arriving at a step: paint it, then set up whatever it needs.
static void step_enter(void) {
  step_draw();
  if (campaign[step].kind == STEP_MINIGAME) {
    // Build the game behind the stripes, then sweep them off it. The clock is
    // armed here but does not tick until the sweep finishes, because the loop
    // skips roundUpdate() while a wipe is running.
    game_start();
    roundStart(ROUND_SECONDS);
    wipeStart(false);
  } else {
    girlSetMood(GIRL_IDLE);
    wipeStart(true);
  }
}

// Leaving a step: hand back whatever step_enter() took. Every start needs a
// matching stop, or the second run through the campaign fails to load.
static void step_exit(void) {
  if (campaign[step].kind == STEP_MINIGAME)
    game_stop();
}

static void step_advance(void) {
  step_exit();
  step++;

  if (step >= CAMPAIGN_LEN) {
    state = GS_MENU;
    wipeCancel();
    menu_draw();
  } else {
    step_enter();
  }
}

// MiniGame -> Should be placed into its own file later
//!!!!
//

static void check_win(void) {
  if (won)
    return;
  for (int i = 0; i < SAW_COUNT; i++)
    if (!placed[i])
      return;
  won = true;
}

// Returns true once every saw is in a slot.
static bool game_update(void) {
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

  return won;
}

static void game_draw(void) {
  // No screen clear: step_draw() has already painted the header for this step.
  printf("\x1b[21;2HDrag the saws onto the marks");
}

static void game_start(void) {
  // Reset Game State.
  for (int i = 0; i < SAW_COUNT; i++) {
    saw_x[i] = saw_reset_x[i];
    saw_y[i] = saw_reset_y[i];
    placed[i] = false;
    slot_taken[i] = false;
  }
  held = -1;
  won = false;

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

  game_draw();
}

// The exact mirror of game_start(), in reverse order: sprites, then VRAM, then
// RAM. NFLib refuses to load into a slot that is still occupied, so skipping
// this halts the ROM the second time the minigame runs.
static void game_stop(void) {
  for (int i = 0; i < SAW_COUNT * 2; i++)
    NF_DeleteSprite(1, i);

  NF_FreeSpriteGfx(1, 0);
  NF_FreeSpriteGfx(1, 1);
  NF_VramSpriteGfxDefrag(1);

  NF_UnloadSpriteGfx(0);
  NF_UnloadSpriteGfx(1);
  NF_UnloadSpritePal(0);
}

static void debug_draw(void) {
  printf("\x1b[0;2Hstate=%s step=%d/%d  ", state == GS_MENU ? "MENU" : "STEP",
         step, CAMPAIGN_LEN);
}

// MAIN -- Most Important
int main(int argc, char **argv) {
  // Top screen: text console.
  videoSetMode(MODE_0_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  PrintConsole top;
  consoleInit(&top, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
  consoleSelect(&top);
  lcdMainOnTop();

  // The portrait is a full-screen sprite. A sprite beats a background at equal
  // priority, so the console is pushed to priority 0 to draw in front of it.
  bgSetPriority(top.bgId, 0);

  // NitroFS must be mounted before NFLib reads any file.
  if (!nitroFSInit(NULL)) {
    perror("nitroFSInit()");
    while (1)
      swiWaitForVBlank();
  }
  NF_SetRootFolder("NITROFS");

  // Bottom screen. NF_Set2D() first, it clears the enable bits the two init
  // calls below set.
  NF_Set2D(1, 0);

  NF_InitTiledBgBuffers();
  NF_InitTiledBgSys(1);

  NF_InitSpriteBuffers();
  NF_InitSpriteSys(1);

  NF_LoadTiledBg("bg/stripes_bg", "stripes", 256, 256);
  NF_CreateTiledBg(1, BG_STRIPES_LAYER, "stripes");
  NF_HideBg(1, BG_STRIPES_LAYER);
  wipeInit(BG_STRIPES_LAYER);

  // Top-screen backdrop, behind the portrait and the text. Stays for the whole
  // run; nothing hides or swaps it.
  topBgInit();

  // Top-screen portrait. Sets up screen 0's sprite engine itself.
  girlInit();
  girlShow(true);

  BG_PALETTE_SUB[0] = RGB15(3, 4, 8);
  menu_draw();

  while (1) {
    scanKeys();

    if (state == GS_MENU) {
      if (keysDown() & KEY_TOUCH) {
        state = GS_STEP;
        step = 0;
        step_enter();
      }
    } else if (state == GS_STEP) {
      if (wipeIsRunning()) {
        // Nothing else runs mid-sweep: no input, and no clock. The round only
        // starts counting once the screen has actually been handed over.
        wipeUpdate();
      } else if (campaign[step].kind == STEP_MINIGAME) {
        // Short-circuit: once the round is over the minigame stops being
        // updated, so a late drag cannot move a saw during the reaction.
        bool won = roundIsPlaying() && game_update();
        RoundPhase phase = roundUpdate(won);
        roundDraw();
        if (phase == ROUND_DONE)
          step_advance();
      } else if (keysDown() & KEY_TOUCH) {
        step_advance();
      }
    }

    debug_draw();
    NF_SpriteOamSet(0);
    NF_SpriteOamSet(1);
    swiWaitForVBlank();
    oamUpdate(&oamMain);
    oamUpdate(&oamSub);
  }

  return 0;
}
