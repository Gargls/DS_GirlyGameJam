// SPDX-License-Identifier: CC0-1.0
//
// The campaign driver.
//
// Everything the player moves through lives here: the title screen, the four
// stages, the life count, the fail-and-restart loop, and the ending. It knows
// about cutscenes and minigames only through their interfaces, and about the
// stages only through the table in campaign.c -- so reordering the game is a
// data edit, never a change here.
//
// THE RESOURCE RULE. Every minigame start() must get exactly one stop(). NFLib
// halts the ROM the moment a game loads into a sprite slot that is still
// occupied, and those slots are a global pool shared by both screens. There are
// two ways a minigame can end -- advancing normally, and running out of lives --
// and both funnel through leave_step() precisely so the count stays at one.

#include <stdbool.h>
#include <stdio.h>

#include <nds.h>

#include <nf_lib.h>

#include "../claude/girl.h"
#include "../claude/music.h"
#include "../claude/round.h"
#include "../claude/timerbar.h"
#include "../claude/topbg.h"
#include "../claude/wipe.h"
#include "campaign.h"
#include "dialogue.h"
#include "run.h"
#include "script.h"

#define LIVES_START 5

// Where a run resumes after the fail cutscene. 1 restarts the stage you died
// on; 0 sends you back to stage 1. One line, deliberately -- see the open
// question in stash/GameDesign.md.
#define RESTART_AT_STAGE_START 1

// Row the minigame's name sits on. round.c overwrites it with the outcome when
// the clock stops, so the two must agree.
#define ROW_NAME 1

// The bottom-screen decorative backdrop (bg/shop_bg, loaded once in main.c).
// It sits behind minigame sprites and in front of nothing that matters during
// a cutscene, so it has to be hidden explicitly whenever the stripes are
// covering -- unlike the stripes layer, it is not in the wipe's window mask.
#define BG_SHOP_LAYER 1

typedef enum {
  RUN_MENU,
  RUN_CUTSCENE,
  RUN_MINIGAME,
  RUN_ENDING,
} RunState;

static RunState state;
static int stage_i;
static int step_i;
static int lives;

// True while the cutscene being played is the fail scene rather than a stage's
// own opener -- it finishes into a restart instead of into the next step.
static bool failing;

// The ending is just a cutscene that happens not to belong to a stage.
static const Cutscene ending_cutscene = {
    .entry = CUT_STAY,
    .mood_in = GIRL_HAPPY,
    .mood_talk = GIRL_HAPPY,
    .lines = script_ending,
    .exit = CUT_REMAIN,
};

static const Stage *cur_stage(void) { return &stages[stage_i]; }
static const Step *cur_step(void) { return &cur_stage()->steps[step_i]; }

// ---- Screens ---------------------------------------------------------------

static void menu_enter(void) {
  state = RUN_MENU;

  wipeCancel();
  timerBarHide();
  NF_HideBg(1, BG_SHOP_LAYER);
  topBgLoad("menu_bg");
  musicPlay("blithe_a");

  girlShow(true);
  girlSetOffset(0, 0);
  girlSetMood(GIRL_IDLE);

  dialogueClear();
  dialogueHud("THE PERFECT DATE", -1);
  dialogueCentred(10, "Press START");
  dialogueCentred(12, "(or touch this screen)");
}

// ---- Steps -----------------------------------------------------------------

// Hand back whatever enter_step() took. Safe to call on a cutscene step, where
// it does nothing.
static void leave_step(void) {
  if (state == RUN_MINIGAME && cur_step()->kind == STEP_MINIGAME)
    cur_step()->game->stop();
}

static void enter_step(void) {
  const Step *s = cur_step();

  dialogueClear();
  dialogueHud(cur_stage()->name, lives);

  if (s->kind == STEP_MINIGAME) {
    state = RUN_MINIGAME;

    if (cur_stage()->shop_bg)
      NF_ShowBg(1, BG_SHOP_LAYER);
    else
      NF_HideBg(1, BG_SHOP_LAYER);

    // Build the game behind the stripes, then sweep them off it. The clock is
    // armed here but does not tick until the sweep has finished, because the
    // loop skips roundUpdate() while a wipe is running -- so no time is lost
    // while the screen is still being handed over.
    s->game->start();
    printf("\x1b[%d;1H%s", ROW_NAME, s->game->name);
    dialogueHint(s->game->hint);

    roundStart(cur_stage()->seconds);
    timerBarSet(roundFramesTotal(), roundFramesTotal());
    wipeStart(false);
  } else {
    state = RUN_CUTSCENE;
    timerBarHide();
    NF_HideBg(1, BG_SHOP_LAYER);

    // Cover the screen with the stripes and play the scene over them. The
    // console's layer is not in the wipe's window mask, so the first page of
    // text stays hidden until the sweep is done, then appears all at once.
    wipeStart(true);
    cutsceneStart(s->cutscene);
  }
}

static void begin_stage(int index) {
  stage_i = index;
  step_i = 0;
  failing = false;
  topBgLoad(stages[index].bg);
  musicPlay(stages[index].music);
  enter_step();
}

static void begin_run(void) {
  lives = LIVES_START;
  begin_stage(0);
}

static void enter_ending(void) {
  state = RUN_ENDING;
  timerBarHide();
  NF_HideBg(1, BG_SHOP_LAYER);

  girlShow(true);
  girlSetOffset(0, 0);

  dialogueClear();
  dialogueHud("THE END", -1);
  wipeStart(true);
  cutsceneStart(&ending_cutscene);
}

// Move to the next step, rolling over into the next stage and then the ending.
static void advance_step(void) {
  leave_step();

  step_i++;
  if (step_i < cur_stage()->count) {
    enter_step();
    return;
  }

  stage_i++;
  if (stage_i >= stage_count) {
    enter_ending();
    return;
  }
  begin_stage(stage_i);
}

// Out of lives. The minigame has already been stopped by the caller.
static void enter_fail(void) {
  state = RUN_CUTSCENE;
  failing = true;
  timerBarHide();
  NF_HideBg(1, BG_SHOP_LAYER);

  // She may be off-screen -- stage 3 sends her away -- and the fail scene needs
  // her back. CUT_STAY in the scene puts her at home and shows her again.
  dialogueClear();
  dialogueHud(cur_stage()->name, 0);
  wipeStart(true);
  cutsceneStart(&fail_cutscene);
}

// ---- Frame -----------------------------------------------------------------

void runInit(void) { menu_enter(); }

void runUpdate(void) {
  // Nothing else runs mid-sweep: no input, no clock, no cutscene. The screen is
  // still being handed over.
  if (wipeIsRunning()) {
    wipeUpdate();
    return;
  }

  switch (state) {
  case RUN_MENU:
    if (keysDown() & (KEY_START | KEY_TOUCH))
      begin_run();
    break;

  case RUN_CUTSCENE:
    if (!cutsceneUpdate())
      break;

    if (!failing) {
      advance_step();
      break;
    }

    // The fail scene has played out. Restart, with a fresh set of lives.
    failing = false;
    lives = LIVES_START;
    begin_stage(RESTART_AT_STAGE_START ? stage_i : 0);
    break;

  case RUN_MINIGAME: {
    // Short-circuit: once the round is over the minigame stops being updated,
    // so a late stylus drag cannot move anything during the reaction hold.
    bool won = roundIsPlaying() && cur_step()->game->update();

    if (roundIsPlaying() && cur_step()->game->draw != NULL)
      cur_step()->game->draw();

    RoundPhase phase = roundUpdate(won);
    roundDraw();
    timerBarSet(roundFramesLeft(), roundFramesTotal());

    if (phase != ROUND_DONE)
      break;

    if (!roundWasWon())
      lives--;

    if (lives > 0) {
      advance_step();
      break;
    }

    // Out of lives. Free the game here, because enter_fail() does not go
    // through advance_step() and this is the only other exit path.
    leave_step();
    enter_fail();
    break;
  }

  case RUN_ENDING:
    if (cutsceneUpdate())
      menu_enter();
    break;
  }
}
