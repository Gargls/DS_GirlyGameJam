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

// Row on the top screen's text console. Near the bottom, over the portrait's
// dark coat, where white text reads well.
#define ROW_CLOCK 22

static RoundPhase phase = ROUND_DONE;
static int frames_left = 0;
static int result_left = 0;
static bool was_won = false;

void roundStart(int seconds) {
  phase = ROUND_PLAYING;
  frames_left = seconds * FPS;
  result_left = 0;
  was_won = false;
  girlSetMood(GIRL_IDLE);
}

bool roundIsPlaying(void) { return phase == ROUND_PLAYING; }

bool roundWasWon(void) { return was_won; }

// Rounds up, so a clock showing "1" still has something left on it and the
// display only reaches 0 when the time is genuinely gone.
int roundSecondsLeft(void) { return (frames_left + FPS - 1) / FPS; }

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
  // Trailing spaces wipe the previous, longer text; printf only paints the
  // characters it writes.
  if (phase == ROUND_PLAYING)
    printf("\x1b[%d;2HTime: %d       ", ROW_CLOCK, roundSecondsLeft());
  else
    printf("\x1b[%d;2H%s", ROW_CLOCK,
           was_won ? "Done in time!  " : "Out of time... ");
}
