// SPDX-License-Identifier: CC0-1.0
//
// Minigame: pay the right amount.
//
// The pile and the price are both dealt fresh each round. Coins can be taken
// back out of the hand again, so overpaying is never a dead end.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"

#define MIN_COINS 6
#define MAX_COINS 9
#define COIN_W 32
#define COIN_H 32

#define MG_GFX 4

// The hand is one 64x64 sprite and its box doubles as the drop zone.
#define HAND_X 188
#define HAND_Y 100
#define HAND_W 64
#define HAND_H 64

// Where the pile is dropped, as the range of a coin's top-left corner. Kept
// clear of the hand so nothing starts already paid.
#define PILE_X0 6
#define PILE_X1 100
#define PILE_Y0 34
#define PILE_Y1 142

// Coins may overlap -- that is what makes it a pile rather than a grid -- but
// the value is printed in the middle of the coin, so a coin covered much past
// this loses its digit and the player is guessing at what is in the pile.
#define PILE_MIN_GAP 18
#define PILE_TRIES 48

static const int denom[3] = {1, 2, 5};

static int coin_count;
static int target;

static int coin_value[MAX_COINS];
static int coin_x[MAX_COINS];
static int coin_y[MAX_COINS];
static int home_x[MAX_COINS];
static int home_y[MAX_COINS];
static bool paid[MAX_COINS];

// The hand has to outrank every coin so it draws behind them, and the coin
// count varies, so its id is only known once the pile is dealt.
static int hand_id;

static int held;
static int grab_dx, grab_dy;
static int total;

// Slots inside the palm: a 3x3 fan, so a handful of coins stays readable.
static int pay_slot_x(int n) { return 192 + (n % 3) * 14; }
static int pay_slot_y(int n) { return 104 + (n / 3) * 14; }

// Deals the pile, then sets the price to the sum of a random subset of it.
// Deriving the price from the coins is what makes every round solvable by
// construction -- there is no search here that could fail.
static void deal(void) {
    coin_count = MIN_COINS + (rand() % (MAX_COINS - MIN_COINS + 1));

    for (int i = 0; i < coin_count; i++)
        coin_value[i] = denom[rand() % 3];

    int order[MAX_COINS];
    for (int i = 0; i < coin_count; i++)
        order[i] = i;
    for (int i = coin_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = order[i];
        order[i] = order[j];
        order[j] = t;
    }

    // Between two coins and all-but-one: never a single coin, and never just
    // tipping the whole pile in.
    int k = 2 + (rand() % (coin_count - 2));
    target = 0;
    for (int i = 0; i < k; i++)
        target += coin_value[order[i]];
}

// Scatters the pile. Rejection sampling keeps a minimum spacing; if a coin
// cannot find room it is placed anyway, which just means a slightly tighter
// heap rather than a failure.
static void scatter(void) {
    for (int i = 0; i < coin_count; i++) {
        int bx = PILE_X0, by = PILE_Y0;

        for (int tries = 0; tries < PILE_TRIES; tries++) {
            bx = PILE_X0 + rand() % (PILE_X1 - PILE_X0 + 1);
            by = PILE_Y0 + rand() % (PILE_Y1 - PILE_Y0 + 1);

            bool clear = true;
            for (int j = 0; j < i; j++) {
                // Every coin is the same size, so comparing corners is the
                // same as comparing centres.
                int dx = bx - home_x[j];
                int dy = by - home_y[j];
                if (dx * dx + dy * dy < PILE_MIN_GAP * PILE_MIN_GAP) {
                    clear = false;
                    break;
                }
            }
            if (clear)
                break;
        }

        home_x[i] = bx;
        home_y[i] = by;
    }
}

static void show_total(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "In hand: %d of %d", total, target);
    gm_status(buf);
}

// Re-lays the paid coins so they fill the fan from the top left with no gap
// where a coin was taken back out.
static void restack(void) {
    int n = 0;
    for (int i = 0; i < coin_count; i++) {
        if (!paid[i])
            continue;
        coin_x[i] = pay_slot_x(n);
        coin_y[i] = pay_slot_y(n);
        NF_MoveSprite(SCR_BOT, i, coin_x[i], coin_y[i]);
        n++;
    }
}

static void recount(void) {
    total = 0;
    for (int i = 0; i < coin_count; i++)
        if (paid[i])
            total += coin_value[i];
    show_total();
}

static bool over_hand(int i) {
    // Test the coin's centre, so a coin only counts once it is properly over
    // the palm rather than just clipping a corner.
    int cx = coin_x[i] + COIN_W / 2;
    int cy = coin_y[i] + COIN_H / 2;
    return cx >= HAND_X && cx < HAND_X + HAND_W && cy >= HAND_Y &&
           cy < HAND_Y + HAND_H;
}

static void coins_start(void) {
    NF_LoadSpriteGfx("sprite/coin1", 0, 32, 32);
    NF_LoadSpriteGfx("sprite/coin2", 1, 32, 32);
    NF_LoadSpriteGfx("sprite/coin5", 2, 32, 32);
    NF_LoadSpriteGfx("sprite/hand", 3, 64, 64);
    NF_LoadSpritePal("sprite/coins", 0);

    for (int i = 0; i < MG_GFX; i++)
        NF_VramSpriteGfx(SCR_BOT, i, i, false);
    NF_VramSpritePal(SCR_BOT, 0, 0);

    deal();
    scatter();

    hand_id = coin_count;
    NF_CreateSprite(SCR_BOT, hand_id, 3, 0, HAND_X, HAND_Y);

    for (int i = 0; i < coin_count; i++) {
        coin_x[i] = home_x[i];
        coin_y[i] = home_y[i];
        paid[i] = false;

        int gfx = coin_value[i] == 5 ? 2 : coin_value[i] == 2 ? 1 : 0;
        NF_CreateSprite(SCR_BOT, i, gfx, 0, coin_x[i], coin_y[i]);
    }

    held = -1;
    total = 0;
    show_total();
}

static bool coins_update(void) {
    if (keysHeld() & KEY_TOUCH) {
        touchPosition t;
        touchRead(&t);

        if (held < 0 && (keysDown() & KEY_TOUCH)) {
            // Forward order picks the lowest id, which is the one on top. In a
            // pile that matters: you always grab the coin you can actually see.
            for (int i = 0; i < coin_count; i++) {
                if (t.px >= coin_x[i] && t.px < coin_x[i] + COIN_W &&
                    t.py >= coin_y[i] && t.py < coin_y[i] + COIN_H) {
                    held = i;
                    grab_dx = t.px - coin_x[i];
                    grab_dy = t.py - coin_y[i];
                    if (paid[i]) {
                        paid[i] = false;
                        recount();
                        restack();
                    }
                    break;
                }
            }
        }

        if (held >= 0) {
            coin_x[held] = gm_clamp(t.px - grab_dx, 0, 256 - COIN_W);
            coin_y[held] = gm_clamp(t.py - grab_dy, 0, 192 - COIN_H);
            NF_MoveSprite(SCR_BOT, held, coin_x[held], coin_y[held]);
        }

        return false;
    }

    // Pen is up. If a coin was being dragged, this frame is the release.
    if (held < 0)
        return false;

    if (over_hand(held)) {
        paid[held] = true;
    } else {
        coin_x[held] = home_x[held];
        coin_y[held] = home_y[held];
    }
    NF_MoveSprite(SCR_BOT, held, coin_x[held], coin_y[held]);
    held = -1;
    restack();
    recount();

    // Only ever settled on release. Winning mid-drag would strand the held
    // coin, because the engine stops calling update() once this returns true.
    return total == target;
}

static void coins_stop(void) {
    // Exactly what start() created, no more: NF_DeleteSprite() hard-errors on
    // an id that was never created, and the coin count varies per round.
    gm_delete_sprites(coin_count + 1);
    gm_free_gfx(MG_GFX);
}

const Minigame mg_coins = {
    .title = "Pay the right amount",
    .hint = "Drag coins into the hand",
    .bg = "shop",
    .start = coins_start,
    .update = coins_update,
    .stop = coins_stop,
};
