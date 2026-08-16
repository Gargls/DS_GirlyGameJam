// SPDX-License-Identifier: CC0-1.0
//
// A timed minigame round.
//
// Two phases. While playing, the clock ticks down and the minigame can win by
// reporting so. When it ends -- won or timed out -- the girl reacts and the
// round holds for a beat, otherwise the reaction would be replaced by the next
// screen before anyone could see it.

#include <stdbool.h>
#include <stdio.h>

#include <nds.h>

#include "girl.h"
#include "round.h"

// The DS runs at 60 frames per second, and this whole file counts in frames.
#define FPS 60

// How long the reaction stays up once the round is over.
#define RESULT_FRAMES (FPS * 3 / 2)

// Row on the bottom-screen console: just under the stage header, where the
// minigame's name sits while it is being played. The old row 22 is now covered
// by the timer bar's sprites, which sit at y=180.
#define ROW_RESULT 1

static RoundPhase phase = ROUND_DONE;
static int frames_left = 0;
static int frames_total = 0;
static int result_left = 0;
static bool was_won = false;

void roundStart(int seconds) {
  phase = ROUND_PLAYING;
  frames_left = seconds * FPS;
  frames_total = frames_left;
  result_left = 0;
  was_won = false;
  girlSetMood(GIRL_IDLE);
}

bool roundIsPlaying(void) { return phase == ROUND_PLAYING; }

bool roundWasWon(void) { return was_won; }

// Rounds up, so a clock showing "1" still has something left on it and the
// display only reaches 0 when the time is genuinely gone.
int roundSecondsLeft(void) { return (frames_left + FPS - 1) / FPS; }

// The timer bar drains off these rather than off whole seconds, so it slides
// smoothly instead of jumping once a second.
int roundFramesLeft(void) { return frames_left; }
int roundFramesTotal(void) { return frames_total; }

static void finish(bool won) {
  was_won = won;
  phase = ROUND_RESULT;
  result_left = RESULT_FRAMES;
  girlSetMood(won ? GIRL_HAPPY : GIRL_DISAPPOINTED);
}

RoundPhase roundUpdate(bool won) {
  switch (phase) {
  case ROUND_PLAYING:
    if (won) {
      finish(true);
      break;
    }
    if (frames_left > 0)
      frames_left--;
    if (frames_left == 0)
      finish(false);
    break;

  case ROUND_RESULT:
    if (result_left > 0)
      result_left--;
    if (result_left == 0)
      phase = ROUND_DONE;
    break;

  case ROUND_DONE:
    break;
  }

  return phase;
}

void roundDraw(void) {
  // While the clock runs, the row belongs to the minigame's name and the time
  // is shown by the bar instead. Only the outcome is painted here, and only
  // once -- redrawing it every frame of the hold would fight the minigame's own
  // draw() for the same row.
  if (phase != ROUND_RESULT || result_left != RESULT_FRAMES)
    return;

  // Trailing spaces wipe whatever longer text was there; printf only paints the
  // characters it writes.
  printf("\x1b[%d;1H%s", ROW_RESULT,
         was_won ? "Done in time!                " : "Out of time...               ");
}
