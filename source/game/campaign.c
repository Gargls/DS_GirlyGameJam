// SPDX-License-Identifier: CC0-1.0
//
// The whole game, as data. This is the file to edit when you want to reorder a
// stage, retime it, or swap a placeholder for the real thing -- adding a
// minigame is a new file in source/mg/ plus one line in a steps[] array here.
// No engine code knows any of these names.
//
// See stash/GameDesign.md for what each stage is meant to be.

#include "../mg/games.h"
#include "campaign.h"
#include "script.h"

// ---- Cutscenes -------------------------------------------------------------
//
// mood_in is what she wears while arriving; mood_talk is switched to just
// before the first line. When they differ you get a visible change of face on
// arrival, which is what stage 1 is after.

static const Cutscene cut_stage1 = {
    .entry = CUT_ENTER_LEFT,
    .mood_in = GIRL_IDLE,
    .mood_talk = GIRL_HAPPY, // idle on the way in, happy once she arrives
    .lines = script_stage1,
    .exit = CUT_REMAIN,
};

static const Cutscene cut_stage2 = {
    .entry = CUT_STAY,
    .mood_in = GIRL_IDLE,
    .mood_talk = GIRL_IDLE, // idle throughout, as specified
    .lines = script_stage2,
    .exit = CUT_REMAIN,
};

static const Cutscene cut_stage3 = {
    .entry = CUT_STAY,
    .mood_in = GIRL_HAPPY, // already happy when the scene opens
    .mood_talk = GIRL_HAPPY,
    .lines = script_stage3,
    .exit = CUT_EXIT_RIGHT, // and she leaves before the stage begins
};

static const Cutscene cut_stage4 = {
    .entry = CUT_ENTER_LEFT, // back in, after being gone for all of stage 3
    .mood_in = GIRL_HAPPY,
    .mood_talk = GIRL_HAPPY,
    .lines = script_stage4,
    .exit = CUT_REMAIN,
};

const Cutscene fail_cutscene = {
    .entry = CUT_STAY,
    .mood_in = GIRL_DISAPPOINTED,
    .mood_talk = GIRL_DISAPPOINTED,
    .lines = script_fail,
    .exit = CUT_REMAIN,
};

// ---- Stages ----------------------------------------------------------------

// Stage 1 -- Getting Ready. Three stand-ins waiting on art.
static const Step steps_stage1[] = {
    {STEP_CUTSCENE, &cut_stage1, 0},
    {STEP_MINIGAME, 0, &placeholderMinigame},
    {STEP_MINIGAME, 0, &placeholderMinigame},
    {STEP_MINIGAME, 0, &placeholderMinigame},
};

// Stage 2 -- Shopping. Real mechanics, wrong theme: the drag puzzle that was
// already here, plus two lifted out of nds-jam-vertical-slice.
static const Step steps_stage2[] = {
    {STEP_CUTSCENE, &cut_stage2, 0},
    {STEP_MINIGAME, 0, &heartsMinigame},
    {STEP_MINIGAME, 0, &starsMinigame},
    {STEP_MINIGAME, 0, &sawMinigame},
};

// Stage 3 -- The Deed. Placeholder art, real mechanics, written for this.
static const Step steps_stage3[] = {
    {STEP_CUTSCENE, &cut_stage3, 0},
    {STEP_MINIGAME, 0, &bopMinigame},
    {STEP_MINIGAME, 0, &cutMinigame},
    {STEP_MINIGAME, 0, &cleanMinigame},
};

// Stage 4 -- The Date. Stand-ins again.
static const Step steps_stage4[] = {
    {STEP_CUTSCENE, &cut_stage4, 0},
    {STEP_MINIGAME, 0, &placeholderMinigame},
    {STEP_MINIGAME, 0, &placeholderMinigame},
    {STEP_MINIGAME, 0, &placeholderMinigame},
};

#define STEPS(a) (a), ((int)(sizeof(a) / sizeof((a)[0])))

// Stage 1 and stage 4 share stage1_bg on purpose -- the same apartment
// building, left at the start of the night and returned to at the end. See
// stash/GameDesign.md for the rest of the backdrop-to-stage reasoning.
const Stage stages[] = {
    {"STAGE 1  Getting Ready", "stage1_bg", "blithe_a", false, 10,
     STEPS(steps_stage1)},
    {"STAGE 2  Shopping", "stage2_bg", "blithe_a", true, 8,
     STEPS(steps_stage2)},
    {"STAGE 3  The Deed", "stage3_bg", "blithe_b", false, 8,
     STEPS(steps_stage3)},
    {"STAGE 4  The Date", "stage1_bg", "blithe_b", false, 6,
     STEPS(steps_stage4)},
};

const int stage_count = (int)(sizeof(stages) / sizeof(stages[0]));
