// SPDX-License-Identifier: CC0-1.0
//
// Plays a Cutscene struct in four phases: enter, talk, exit, done.
//
// The slide uses a quarter turn of sine rather than a straight line, so she
// decelerates into place instead of stopping dead. sinLerp() takes an angle
// where 32768 is a full turn and returns a fixed-point value scaled by 4096,
// so a quarter turn is 8192 and the result runs 0 -> 4096.

#include <stdbool.h>

#include <nds.h>

#include "../claude/girl.h"
#include "cutscene.h"
#include "dialogue.h"

#define SLIDE_FRAMES 34

typedef enum { PH_ENTER, PH_TALK, PH_EXIT, PH_DONE } Phase;

static const Cutscene *scene;
static Phase phase;
static int frame;
static int line;
static int from_x, to_x;

// Ease-out interpolation along the quarter sine described above.
static int ease(int from, int to, int f, int total) {
  if (f >= total)
    return to;
  int s = sinLerp((f * 8192) / total); // 0 .. 4096
  return from + (((to - from) * s) >> 12);
}

static void show_line(void) {
  // The next entry being non-NULL is exactly "there is more after this", which
  // is also what the prompt at the bottom of the box needs to know.
  dialoguePage(scene->lines[line], scene->lines[line + 1] != NULL);
}

// Moves on to talking. Split out because two phases can lead here: a scene with
// an entrance, and one without.
static void begin_talk(void) {
  girlSetOffset(0, 0);
  girlSetMood(scene->mood_talk);
  phase = PH_TALK;
  line = 0;
  show_line();
}

void cutsceneStart(const Cutscene *c) {
  scene = c;
  frame = 0;
  line = 0;

  switch (c->entry) {
  case CUT_ENTER_LEFT:
    girlShow(true);
    girlSetMood(c->mood_in);
    from_x = GIRL_OFF_LEFT;
    to_x = 0;
    girlSetOffset(from_x, 0);
    phase = PH_ENTER;
    break;

  case CUT_STAY:
    girlShow(true);
    girlSetMood(c->mood_in);
    girlSetOffset(0, 0);
    begin_talk();
    break;

  case CUT_ABSENT:
    girlShow(false);
    phase = PH_TALK;
    show_line();
    break;
  }
}

bool cutsceneUpdate(void) {
  switch (phase) {
  case PH_ENTER:
    frame++;
    girlSetOffset(ease(from_x, to_x, frame, SLIDE_FRAMES), 0);
    if (frame >= SLIDE_FRAMES)
      begin_talk();
    return false;

  case PH_TALK:
    if (!(keysDown() & KEY_TOUCH))
      return false;

    line++;
    if (scene->lines[line] != NULL) {
      show_line();
      return false;
    }

    // Out of lines. Clear the box before any exit animation, so she is not
    // sliding away underneath stale text.
    dialogueClearBox();

    if (scene->exit == CUT_EXIT_RIGHT && scene->entry != CUT_ABSENT) {
      from_x = 0;
      to_x = GIRL_OFF_RIGHT;
      frame = 0;
      phase = PH_EXIT;
      return false;
    }
    phase = PH_DONE;
    return true;

  case PH_EXIT:
    frame++;
    girlSetOffset(ease(from_x, to_x, frame, SLIDE_FRAMES), 0);
    if (frame < SLIDE_FRAMES)
      return false;

    // Park her off-screen but also hide her, so nothing can make her flicker
    // back into view during the minigames that follow.
    girlShow(false);
    girlSetOffset(0, 0);
    phase = PH_DONE;
    return true;

  case PH_DONE:
  default:
    return true;
  }
}
