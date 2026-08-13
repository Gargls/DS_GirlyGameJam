// SPDX-License-Identifier: CC0-1.0
//
// Minigame: hammer the nails.
//
// Tap-to-repeat, so it plays differently from the two dragging games. Each nail
// takes three hits. The nail art is a 4-frame strip and all five nails share
// one VRAM slot: NF_CreateSprite() gives every sprite its own frame counter, so
// they sink independently.

#include <stdbool.h>
#include <stdio.h>

#include "game.h"

#define NAIL_COUNT 5
#define NAIL_W 16
#define NAIL_H 32
#define HITS_NEEDED 3

#define ID_HAMMER 0
#define ID_NAIL0 1
#define MG_SPRITES 6
#define MG_GFX 2

// Frames the hammer stays visible after a hit.
#define HAMMER_FRAMES 7

// Taps are forgiving: the head is only a few pixels wide, so the hit box is
// grown well past the sprite on every side.
#define GRAB_PAD 7

static const int nail_x[NAIL_COUNT] = {28, 110, 196, 56, 150};
static const int nail_y[NAIL_COUNT] = {30, 22, 44, 108, 120};

static int hits[NAIL_COUNT];
static int hammer_timer;

static void show_progress(void) {
    char buf[32];
    int done = 0;
    for (int i = 0; i < NAIL_COUNT; i++)
        if (hits[i] >= HITS_NEEDED)
            done++;
    snprintf(buf, sizeof(buf), "Driven: %d of %d", done, NAIL_COUNT);
    gm_status(buf);
}

static void nails_start(void) {
    // 16x32 is the size of one frame, not of the file. The file holds four of
    // them stacked, which is how NFLib works out lastframe.
    NF_LoadSpriteGfx("sprite/nail", 0, 16, 32);
    NF_LoadSpriteGfx("sprite/hammer", 1, 32, 32);
    NF_LoadSpritePal("sprite/props", 0);

    for (int i = 0; i < MG_GFX; i++)
        NF_VramSpriteGfx(SCR_BOT, i, i, false);
    NF_VramSpritePal(SCR_BOT, 0, 0);

    NF_CreateSprite(SCR_BOT, ID_HAMMER, 1, 0, 0, 0);
    NF_ShowSprite(SCR_BOT, ID_HAMMER, false);
    hammer_timer = 0;

    for (int i = 0; i < NAIL_COUNT; i++) {
        hits[i] = 0;
        NF_CreateSprite(SCR_BOT, ID_NAIL0 + i, 0, 0, nail_x[i], nail_y[i]);
    }

    show_progress();
}

static void strike(int i) {
    hits[i]++;
    NF_SpriteFrame(SCR_BOT, ID_NAIL0 + i,
                   hits[i] < HITS_NEEDED ? hits[i] : HITS_NEEDED);

    int hx = nail_x[i] - 6;
    int hy = nail_y[i] - 18;
    if (hy < 0)
        hy = 0;
    NF_MoveSprite(SCR_BOT, ID_HAMMER, hx, hy);
    NF_ShowSprite(SCR_BOT, ID_HAMMER, true);
    hammer_timer = HAMMER_FRAMES;

    show_progress();
}

static bool nails_update(void) {
    if (hammer_timer > 0) {
        hammer_timer--;
        if (hammer_timer == 0)
            NF_ShowSprite(SCR_BOT, ID_HAMMER, false);
    }

    if (keysDown() & KEY_TOUCH) {
        touchPosition t;
        touchRead(&t);

        for (int i = 0; i < NAIL_COUNT; i++) {
            if (hits[i] >= HITS_NEEDED)
                continue;
            if (t.px >= nail_x[i] - GRAB_PAD &&
                t.px < nail_x[i] + NAIL_W + GRAB_PAD &&
                t.py >= nail_y[i] - GRAB_PAD &&
                t.py < nail_y[i] + NAIL_H + GRAB_PAD) {
                strike(i);
                break;
            }
        }
    }

    for (int i = 0; i < NAIL_COUNT; i++)
        if (hits[i] < HITS_NEEDED)
            return false;

    // Let the last hammer flash finish before handing back to the engine.
    return hammer_timer == 0;
}

static void nails_stop(void) {
    gm_delete_sprites(MG_SPRITES);
    gm_free_gfx(MG_GFX);
}

const Minigame mg_nails = {
    .title = "Hammer the nails",
    .hint = "Tap each nail three times",
    .bg = "wall",
    .start = nails_start,
    .update = nails_update,
    .stop = nails_stop,
};
