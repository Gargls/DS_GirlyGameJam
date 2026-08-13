// SPDX-License-Identifier: CC0-1.0
//
// A timed minigame round: a countdown, then a moment to show the result.

#ifndef CLAUDE_ROUND_H
#define CLAUDE_ROUND_H

#include <stdbool.h>

typedef enum {
  ROUND_PLAYING, ///< Clock running, the minigame owns the stylus.
  ROUND_RESULT,  ///< Over. Holding on the reaction so it can be read.
  ROUND_DONE     ///< Finished; the campaign may move on.
} RoundPhase;

/// Begins a round of the given length and puts the girl back to idle.
void roundStart(int seconds);

/// True while the clock is running. Use it to decide whether to call the
/// minigame's update at all -- once the round is over it must stop reacting to
/// the stylus.
bool roundIsPlaying(void);

/// Advances one frame. Pass whether the minigame reports itself won. Returns
/// the phase; ROUND_DONE means the result has been shown long enough.
RoundPhase roundUpdate(bool won);

/// Paints the clock, or the outcome once the clock has stopped.
void roundDraw(void);

int roundSecondsLeft(void);
bool roundWasWon(void);

#endif // CLAUDE_ROUND_H
