// SPDX-License-Identifier: CC0-1.0
//
// The whole game, as data. See campaign.c.

#ifndef GAME_CAMPAIGN_H
#define GAME_CAMPAIGN_H

#include <stdbool.h>

#include "cutscene.h"
#include "minigame.h"

typedef enum { STEP_CUTSCENE, STEP_MINIGAME } StepKind;

typedef struct {
  StepKind kind;
  const Cutscene *cutscene; ///< Set when kind is STEP_CUTSCENE.
  const Minigame *game;     ///< Set when kind is STEP_MINIGAME.
} Step;

typedef struct {
  const char *name;
  const char *bg;  ///< nitrofiles/bg/<bg>.{img,map,pal}, shown on the top
                   ///< screen for the whole stage.
  const char *music; ///< nitrofiles/audio/<music>.wav, looped for the whole
                     ///< stage. Unchanged if it matches what is already
                     ///< playing, so back-to-back stages sharing a track
                     ///< never restart it.
  bool shop_bg;    ///< Show bg/shop_bg behind this stage's minigames, on the
                   ///< bottom screen.
  int seconds;     ///< Clock for every minigame in this stage.
  const Step *steps;
  int count;
} Stage;

extern const Stage stages[];
extern const int stage_count;

/// Played when the last life is lost, before the stage restarts.
extern const Cutscene fail_cutscene;

#endif // GAME_CAMPAIGN_H
