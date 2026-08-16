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
// mood_in is what she wears while arriving. What she wears while talking is
// per-line now -- see script.c -- so a scene only needs to say this once, at
// the door, rather than declare a mood for the whole conversation.

static const Cutscene cut_stage1 = {
    .entry = CUT_ENTER_LEFT,
    .mood_in = GIRL_IDLE, // idle on the way in; script_stage1 takes it happy
    .lines = script_stage1,
    .exit = CUT_REMAIN,
};

static const Cutscene cut_stage2 = {
    .entry = CUT_STAY,
    .mood_in = GIRL_IDLE,
    .lines = script_stage2,
    .exit = CUT_REMAIN,
};

static const Cutscene cut_stage3 = {
    .entry = CUT_STAY,
    .mood_in = GIRL_HAPPY, // already happy when the scene opens
    .lines = script_stage3,
    .exit = CUT_EXIT_RIGHT, // and she leaves before the stage begins
};

static const Cutscene cut_stage4 = {
    .entry = CUT_ENTER_LEFT, // back in, after being gone for all of stage 3
    .mood_in = GIRL_HAPPY,
    .lines = script_stage4,
    .exit = CUT_REMAIN,
};

const Cutscene fail_cutscene = {
    .entry = CUT_STAY,
    .mood_in = GIRL_DISAPPOINTED,
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

// Stage 2 -- Shopping. Build-a-Bear, in two parts: the eyes, then the bow.
static const Step steps_stage2[] = {
    {STEP_CUTSCENE, &cut_stage2, 0},
    {STEP_MINIGAME, 0, &eyesMinigame},
    {STEP_MINIGAME, 0, &bowMinigame},
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
    {"STAGE 1  Getting Ready", "stage1_bg", "blithe_a", 10,
     STEPS(steps_stage1)},
    {"STAGE 2  Shopping", "stage2_bg", "blithe_a", 8,
     STEPS(steps_stage2)},
    {"STAGE 3  The Deed", "stage3_bg", "blithe_b", 8,
     STEPS(steps_stage3)},
    {"STAGE 4  The Date", "stage1_bg", "blithe_b", 6,
     STEPS(steps_stage4)},
};

const int stage_count = (int)(sizeof(stages) / sizeof(stages[0]));
