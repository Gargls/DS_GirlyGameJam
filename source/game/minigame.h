// SPDX-License-Identifier: CC0-1.0
//
// The interface every minigame implements. The campaign driver in run.c talks
// to a game only through this handle -- it never learns what the game does, so
// adding one is a new file in source/mg/ plus a single line in campaign.c.

#ifndef GAME_MINIGAME_H
#define GAME_MINIGAME_H

#include <stdbool.h>

typedef struct {
  /// Shown in the header while the game is being played.
  const char *name;

  /// One-line instruction, drawn under the header. Keep it under 30 characters
  /// or it wraps off the console.
  const char *hint;

  /// Load sprites and reset state.
  ///
  /// Paired with stop(): every start MUST have exactly one matching stop, or
  /// NFLib halts the ROM the next time a game loads into a slot that is still
  /// occupied. Sprite RAM slots are a *global* pool shared by both screens, so
  /// this is not a per-screen accident you can get away with.
  void (*start)(void);

  /// One frame of input and logic. Returns true on the frame the game is won.
  /// Only called while the round clock is actually running.
  bool (*update)(void);

  /// Optional per-frame repaint, for games with live feedback such as a hit
  /// counter. May be NULL when the hint drawn at start never changes.
  void (*draw)(void);

  /// Undo everything start() built, in reverse order.
  void (*stop)(void);
} Minigame;

#endif // GAME_MINIGAME_H
