// SPDX-License-Identifier: CC0-1.0
//
// Minigame: saw the logs.
//
// Drag-to-scrub rather than drag-and-drop. Holding the saw over a log's cut
// mark and working it back and forth banks distance; bank enough and the log
// falls in two. Only travel counts, so parking the saw on the mark does
// nothing.

#include <stdbool.h>
#include <stdio.h>

#include "game.h"

#define LOG_COUNT 3
#define LOG_W 64
#define LOG_H 32
#define LOG_Y 130

#define SAW_W 64
#define SAW_H 32
#define SAW_HOME_X 170
#define SAW_HOME_Y 36

// Sprite ids. The saw is 0 so it always draws over the logs.
#define ID_SAW 0
#define ID_RIGHT0 1
#define ID_LEFT0 4
#define MG_SPRITES 7
#define MG_GFX 3

// Pixels of saw travel needed to get through one log.
#define CUT_WORK 150
// How far the blade may sit from the cut mark and still bite.
#define SEAM_TOL 14
// A single frame cannot bank more than this, so flinging the pen across the
// screen is no faster than actually sawing.
#define MAX_STEP 16

#define BAR 12

// Spaced apart on purpose. Butted together the three logs read as one long
// log with six end faces instead of three separate jobs.
static const int log_x[LOG_COUNT] = {6, 92, 178};

static int work[LOG_COUNT];
static bool cut[LOG_COUNT];
static int fall_y[LOG_COUNT];
static int fall_v[LOG_COUNT];

static int saw_x, saw_y;
static bool held;
static int grab_dx, grab_dy;

static void show_progress(void) {
    char buf[40], bar[BAR + 1];

    int done = 0, best = 0;
    for (int i = 0; i < LOG_COUNT; i++) {
        if (cut[i])
            done++;
        else if (work[i] > best)
            best = work[i];
    }

    int filled = (best * BAR) / CUT_WORK;
    for (int i = 0; i < BAR; i++)
        bar[i] = i < filled ? '#' : '.';
    bar[BAR] = '\0';

    snprintf(buf, sizeof(buf), "Cut %d/%d  [%s]", done, LOG_COUNT, bar);
    gm_status(buf);
}

// The teeth are along the bottom of the blade, a little left of centre.
static int blade_x(void) { return saw_x + 24; }
static int blade_y(void) { return saw_y + 22; }

static void apply_cut(int travel) {
    bool changed = false;

    for (int i = 0; i < LOG_COUNT; i++) {
        if (cut[i])
            continue;

        int seam = log_x[i] + LOG_W / 2;
        int dx = blade_x() - seam;
        if (dx < 0)
            dx = -dx;
        if (dx > SEAM_TOL)
            continue;
        if (blade_y() < LOG_Y - 4 || blade_y() > LOG_Y + LOG_H + 4)
            continue;

        work[i] += travel;
        changed = true;

        if (work[i] >= CUT_WORK) {
            work[i] = CUT_WORK;
            cut[i] = true;
        }
    }

    if (changed)
        show_progress();
}

static void saw_start(void) {
    NF_LoadSpriteGfx("sprite/log_l", 0, 32, 32);
    NF_LoadSpriteGfx("sprite/log_r", 1, 32, 32);
    NF_LoadSpriteGfx("sprite/bigsaw", 2, 64, 32);
    NF_LoadSpritePal("sprite/props", 0);

    for (int i = 0; i < MG_GFX; i++)
        NF_VramSpriteGfx(SCR_BOT, i, i, false);
    NF_VramSpritePal(SCR_BOT, 0, 0);

    saw_x = SAW_HOME_X;
    saw_y = SAW_HOME_Y;
    held = false;
    NF_CreateSprite(SCR_BOT, ID_SAW, 2, 0, saw_x, saw_y);

    for (int i = 0; i < LOG_COUNT; i++) {
        work[i] = 0;
        cut[i] = false;
        fall_y[i] = LOG_Y;
        fall_v[i] = 0;
        NF_CreateSprite(SCR_BOT, ID_RIGHT0 + i, 1, 0, log_x[i] + 32, LOG_Y);
        NF_CreateSprite(SCR_BOT, ID_LEFT0 + i, 0, 0, log_x[i], LOG_Y);
    }

    show_progress();
}

static bool saw_update(void) {
    if (keysHeld() & KEY_TOUCH) {
        touchPosition t;
        touchRead(&t);

        if (!held && (keysDown() & KEY_TOUCH)) {
            if (t.px >= saw_x && t.px < saw_x + SAW_W && t.py >= saw_y &&
                t.py < saw_y + SAW_H) {
                held = true;
                grab_dx = t.px - saw_x;
                grab_dy = t.py - saw_y;
            }
        }

        if (held) {
            int nx = gm_clamp(t.px - grab_dx, 0, 256 - SAW_W);
            int ny = gm_clamp(t.py - grab_dy, 0, 192 - SAW_H);

            int travel = nx - saw_x;
            if (travel < 0)
                travel = -travel;
            if (travel > MAX_STEP)
                travel = MAX_STEP;

            saw_x = nx;
            saw_y = ny;
            NF_MoveSprite(SCR_BOT, ID_SAW, saw_x, saw_y);

            if (travel > 0)
                apply_cut(travel);
        }
    } else {
        held = false;
    }

    // Cut halves drop off the bench. The win waits for them to clear the
    // screen, otherwise the engine would stop updating mid-fall and leave a
    // log hanging in the air.
    bool all_gone = true;
    for (int i = 0; i < LOG_COUNT; i++) {
        if (!cut[i]) {
            all_gone = false;
            continue;
        }
        if (fall_y[i] > 200)
            continue;

        all_gone = false;
        if (fall_v[i] < 10)
            fall_v[i]++;
        fall_y[i] += fall_v[i];
        NF_MoveSprite(SCR_BOT, ID_RIGHT0 + i, log_x[i] + 32 + (fall_v[i] / 2),
                      fall_y[i]);

        if (fall_y[i] > 200)
            NF_ShowSprite(SCR_BOT, ID_RIGHT0 + i, false);
    }

    return all_gone;
}

static void saw_stop(void) {
    gm_delete_sprites(MG_SPRITES);
    gm_free_gfx(MG_GFX);
}

const Minigame mg_saw = {
    .title = "Saw the logs",
    .hint = "Work the saw across each mark",
    .bg = "bench",
    .start = saw_start,
    .update = saw_update,
    .stop = saw_stop,
};
