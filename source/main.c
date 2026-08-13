// SPDX-License-Identifier: CC0-1.0
//
// Sawmill Party -- engine and campaign flow.
//
// The top screen is a text layer over a backdrop and never changes structure.
// The bottom screen swaps its background per minigame and owns all the sprites.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <filesystem.h>
#include <nds.h>

#include <nf_lib.h>

#include "game.h"

// The campaign, in order. Adding a minigame means adding one entry here.
static const Minigame *const campaign[] = {
    &mg_coins,
    &mg_saw,
    &mg_nails,
};
#define CAMPAIGN_LEN ((int)(sizeof(campaign) / sizeof(campaign[0])))

// Backdrop shown on the bottom screen while the menu is up.
#define MENU_BOT_BG "bench"

// Between-games wipe. The barber-pole pattern sits on its own background layer,
// and a hardware window decides per pixel column whether the screen shows that
// layer or the scene underneath it.
#define STRIPE_LAYER 2
#define WIPE_FRAMES 22
#define WIPE_SCROLL 2

// Bits of REG_WININ_SUB / REG_WINOUT_SUB: one per background layer, plus bit 4
// for sprites. Letting the window hide sprites too is what makes this work
// without having to touch a single sprite priority.
#define WIN_STRIPES (1 << STRIPE_LAYER)
#define WIN_SCENE ((1 << BG_LAYER) | (1 << 4))

typedef enum { ST_MENU, ST_PLAY, ST_CLEARED, ST_DONE, ST_WIPE } State;

static State state = ST_MENU;
static int current = 0;

// Frames since boot. The only entropy the DS offers here for free: the player
// sits on the menu for an arbitrary number of frames before touching, so this
// is a serviceable seed.
static unsigned ticks = 0;

// ---------------------------------------------------------------- helpers ---

void gm_status(const char *text) {
    char buf[32];
    // Left aligned in a fixed width, so a shorter line wipes the longer one it
    // replaces. NF_WriteText() only paints the tiles it covers.
    snprintf(buf, sizeof(buf), "%-28s", text);
    NF_WriteText(SCR_TOP, TXT_LAYER, 2, ROW_STATUS, buf);
}

void gm_delete_sprites(int count) {
    for (int i = 0; i < count; i++)
        NF_DeleteSprite(SCR_BOT, i);
}

void gm_free_gfx(int count) {
    for (int i = 0; i < count; i++)
        NF_FreeSpriteGfx(SCR_BOT, i);

    // Freeing leaves holes behind. Without this the next minigame's graphics
    // can fail to find one contiguous block even though enough VRAM is free.
    NF_VramSpriteGfxDefrag(SCR_BOT);

    for (int i = 0; i < count; i++)
        NF_UnloadSpriteGfx(i);

    NF_UnloadSpritePal(0);
}

// ------------------------------------------------------------------ flow ---

static void set_bottom_bg(const char *name) {
    NF_DeleteTiledBg(SCR_BOT, BG_LAYER);
    NF_CreateTiledBg(SCR_BOT, BG_LAYER, name);
}

static void menu_enter(void) {
    NF_ClearTextLayer(SCR_TOP, TXT_LAYER);
    NF_WriteText(SCR_TOP, TXT_LAYER, 6, 8, "S A W M I L L   P A R T Y");
    NF_WriteText(SCR_TOP, TXT_LAYER, 4, 11, "Three jobs. Get them done.");
    NF_WriteText(SCR_TOP, TXT_LAYER, 3, 14, "Touch lower screen to start");
    set_bottom_bg(MENU_BOT_BG);
    current = 0;
    state = ST_MENU;
}

static void game_enter(int index) {
    const Minigame *g = campaign[index];
    char buf[32];

    NF_ClearTextLayer(SCR_TOP, TXT_LAYER);
    snprintf(buf, sizeof(buf), "Job %d of %d", index + 1, CAMPAIGN_LEN);
    NF_WriteText(SCR_TOP, TXT_LAYER, 2, ROW_COUNTER, buf);
    NF_WriteText(SCR_TOP, TXT_LAYER, 2, ROW_TITLE, g->title);
    NF_WriteText(SCR_TOP, TXT_LAYER, 2, ROW_HINT, g->hint);

    set_bottom_bg(g->bg);
    g->start();

    current = index;
    state = ST_PLAY;
}

static void done_enter(void) {
    NF_ClearTextLayer(SCR_TOP, TXT_LAYER);
    NF_WriteText(SCR_TOP, TXT_LAYER, 7, 9, "All jobs finished!");
    NF_WriteText(SCR_TOP, TXT_LAYER, 4, 12, "Touch to return to menu");
    set_bottom_bg(MENU_BOT_BG);
    state = ST_DONE;
}

// ------------------------------------------------------------------ wipe ---

static void (*wipe_midpoint)(void);
static State wipe_after;
static int wipe_frame;
static int wipe_scroll;
static bool wipe_covering;

// Sets the window so columns [0, w) show one thing and the rest shows the
// other. Both halves of the transition use the same left-anchored rectangle
// growing 0 -> 256; only the inside/outside meanings swap. That is what makes
// the stripes fill from the left and then clear from the left as well, rather
// than clearing back the way they came.
static void wipe_apply(int w, bool covering) {
    u16 inside = covering ? WIN_STRIPES : WIN_SCENE;
    u16 outside = covering ? WIN_SCENE : WIN_STRIPES;

    if (w <= 0) {
        // An empty rectangle cannot be expressed: the hardware reads X0 == X1
        // as undefined, not as zero width. Drive both regions instead.
        REG_WININ_SUB = outside;
        REG_WINOUT_SUB = outside;
    } else if (w >= SCREEN_WIDTH) {
        // Nor can a full-width one, since the right edge is only 8 bits.
        REG_WININ_SUB = inside;
        REG_WINOUT_SUB = inside;
    } else {
        SUB_WIN0_X0 = 0;
        SUB_WIN0_X1 = (u8)w;
        SUB_WIN0_Y0 = 0;
        SUB_WIN0_Y1 = SCREEN_HEIGHT;
        REG_WININ_SUB = inside;
        REG_WINOUT_SUB = outside;
    }
}

static void wipe_begin(void (*midpoint)(void)) {
    wipe_midpoint = midpoint;
    wipe_frame = 0;
    wipe_covering = true;
    state = ST_WIPE;

    NF_ShowBg(SCR_BOT, STRIPE_LAYER);
    REG_DISPCNT_SUB |= DISPLAY_WIN0_ON;
    wipe_apply(0, true);
}

static void wipe_update(void) {
    // The pattern is periodic in X, so scrolling the layer is the whole
    // animation -- it is the same barber-pole crawl the software renderer got
    // by offsetting each scanline every frame.
    wipe_scroll = (wipe_scroll + WIPE_SCROLL) & 255;
    NF_ScrollBg(SCR_BOT, STRIPE_LAYER, wipe_scroll, 0);

    wipe_frame++;
    wipe_apply((wipe_frame * SCREEN_WIDTH) / WIPE_FRAMES, wipe_covering);

    if (wipe_frame < WIPE_FRAMES)
        return;

    if (wipe_covering) {
        // Fully covered. Everything swaps over here, out of sight: the old
        // minigame tears down, the background changes and the next one builds.
        // Whatever state that lands in is resumed once the screen clears.
        wipe_midpoint();
        wipe_after = state;
        state = ST_WIPE;
        wipe_covering = false;
        wipe_frame = 0;
    } else {
        REG_DISPCNT_SUB &= ~DISPLAY_WIN0_ON;
        NF_HideBg(SCR_BOT, STRIPE_LAYER);
        state = wipe_after;
    }
}

static void act_start_campaign(void) {
    // Seeded here rather than at boot, so the count is whatever the player's
    // first touch happened to land on.
    srand(ticks);
    game_enter(0);
}

static void act_advance(void) {
    campaign[current]->stop();
    if (current + 1 < CAMPAIGN_LEN)
        game_enter(current + 1);
    else
        done_enter();
}

// ------------------------------------------------------------------ setup ---

static void load_assets(void) {
    NF_LoadTiledBg("bg/menu_bg", "menu", 256, 256);
    NF_LoadTiledBg("bg/coin_bg", "shop", 256, 256);
    NF_LoadTiledBg("bg/bench_bg", "bench", 256, 256);
    NF_LoadTiledBg("bg/wall_bg", "wall", 256, 256);
    NF_LoadTiledBg("bg/stripes_bg", "stripes", 256, 256);
    NF_LoadTextFont("fnt/smallfont", "small", 256, 256, 0);
}

int main(int argc, char **argv) {
    // Mode 0 (tiled) on both screens, before anything else: the subsystems
    // below only OR their enable bits into REG_DISPCNT.
    NF_Set2D(SCR_TOP, 0);
    NF_Set2D(SCR_BOT, 0);

    // NitroFS must be mounted before NFLib reads any file.
    if (!nitroFSInit(NULL)) {
        consoleDemoInit();
        perror("nitroFSInit()");
        while (1)
            swiWaitForVBlank();
    }
    NF_SetRootFolder("NITROFS");

    // Backgrounds: VRAM_A + VRAM_E on the top screen, VRAM_C + VRAM_H on the
    // bottom. The E/H banks hold extended palettes, which is what lets a
    // 256-colour background and the font each keep a full palette of their own.
    NF_InitTiledBgBuffers();
    NF_InitTiledBgSys(SCR_TOP);
    NF_InitTiledBgSys(SCR_BOT);

    NF_InitTextSys(SCR_TOP);

    // Sprites are bottom screen only: VRAM_D + VRAM_I, no clash with the above.
    NF_InitSpriteBuffers();
    NF_InitSpriteSys(SCR_BOT);

    load_assets();

    // The top screen keeps this backdrop and text layer for the whole run.
    NF_CreateTiledBg(SCR_TOP, BG_LAYER, "menu");
    NF_CreateTextLayer(SCR_TOP, TXT_LAYER, 0, "small");

    // Something has to exist on the bottom layer before set_bottom_bg() can
    // delete it.
    NF_CreateTiledBg(SCR_BOT, BG_LAYER, MENU_BOT_BG);

    // The wipe layer is created once and then only shown and hidden. It sits in
    // front of the scene by virtue of its lower layer number, so it stays
    // hidden except while a transition is running.
    NF_CreateTiledBg(SCR_BOT, STRIPE_LAYER, "stripes");
    NF_HideBg(SCR_BOT, STRIPE_LAYER);

    menu_enter();

    while (1) {
        scanKeys();
        ticks++;

        // One state per frame. A touch that ends a state is therefore never
        // seen by the state it moves into, which is what stops the tap that
        // starts a minigame from also grabbing whatever is under the pen.
        switch (state) {
        case ST_MENU:
            if (keysDown() & KEY_TOUCH)
                wipe_begin(act_start_campaign);
            break;

        case ST_PLAY:
            if (campaign[current]->update()) {
                state = ST_CLEARED;
                NF_WriteText(SCR_TOP, TXT_LAYER, 2, ROW_PROMPT,
                             "Done! Touch to carry on");
            }
            break;

        case ST_CLEARED:
            if (keysDown() & KEY_TOUCH)
                wipe_begin(act_advance);
            break;

        case ST_DONE:
            if (keysDown() & KEY_TOUCH)
                wipe_begin(menu_enter);
            break;

        case ST_WIPE:
            wipe_update();
            break;
        }

        NF_SpriteOamSet(SCR_BOT);
        NF_UpdateTextLayers();
        swiWaitForVBlank();
        oamUpdate(&oamSub);
    }

    return 0;
}
