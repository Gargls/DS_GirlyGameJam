// SPDX-License-Identifier: CC0-1.0
//
// A cutscene is data, not code: a struct in the campaign table describing where
// the girl comes from, what mood she is in, what she says, and where she goes.
// cutscene.c plays it. Adding a scene never means writing logic.

#ifndef GAME_CUTSCENE_H
#define GAME_CUTSCENE_H

#include <stdbool.h>

#include "../claude/girl.h"
#include "script.h"

typedef enum {
  CUT_ENTER_LEFT, ///< Slides in from off the left edge to the middle.
  CUT_STAY,       ///< Already on screen; no movement.
  CUT_ABSENT      ///< Not in this scene at all.
} CutEntry;

typedef enum {
  CUT_REMAIN,    ///< Stays on screen when the scene ends.
  CUT_EXIT_RIGHT ///< Slides off the right edge before the scene ends.
} CutExit;

typedef struct {
  CutEntry entry;
  GirlMood mood_in; ///< Mood while she arrives, before the first line shows.

  /// NULL-terminated (by `.text`). See script.c -- the terminator is what
  /// ends the scene, so there is no count here to fall out of step with the
  /// writing. Each line carries its own mood, applied the moment it is shown
  /// -- that is what replaced the old single mood_talk, one mood for the
  /// whole scene.
  const Line *lines;

  CutExit exit;
} Cutscene;

/// Begins a scene. The girl is placed at her starting position immediately, so
/// this is safe to call behind a wipe.
void cutsceneStart(const Cutscene *c);

/// One frame. Returns true on the frame the scene has finished.
bool cutsceneUpdate(void);

#endif // GAME_CUTSCENE_H
