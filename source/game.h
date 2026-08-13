// SPDX-License-Identifier: CC0-1.0
//
// Shared interface between the engine (main.c) and the individual minigames.

#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include <nds.h>

#include <nf_lib.h>

// The top screen is text over a backdrop, the bottom screen is the play field.
// Sprites only ever exist on the bottom screen.
#define SCR_TOP 0
#define SCR_BOT 1

#define TXT_LAYER 0
#define BG_LAYER 3

// Rows on the top screen's text layer, in tiles. The engine owns the counter,
// title, hint and prompt; a minigame only ever writes the status line.
#define ROW_COUNTER 2
#define ROW_TITLE 4
#define ROW_HINT 6
#define ROW_STATUS 9
#define ROW_PROMPT 12

/// One microgame. The engine calls start() once, update() every frame until it
/// returns true, then stop() once the player acknowledges the win.
typedef struct {
    const char *title;  ///< Shown on the top screen. Max 30 characters.
    const char *hint;   ///< One line of instructions, under the title.
    const char *bg;     ///< Name of the tiled background for the bottom screen.

    void (*start)(void);   ///< Load graphics, create sprites, reset state.
    bool (*update)(void);  ///< One frame. Returns true when the goal is met.
    void (*stop)(void);    ///< Tear down everything start() allocated.
} Minigame;

extern const Minigame mg_coins;
extern const Minigame mg_saw;
extern const Minigame mg_nails;

/// Keeps a dragged sprite on screen. Worth doing for the Y axis in particular:
/// the OAM Y field is unsigned 8-bit, so a sprite dragged above the top edge
/// does not clip, it wraps around and reappears at the bottom.
static inline int gm_clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/// Writes the minigame's progress line on the top screen, padded so a shorter
/// string always erases whatever was there before.
void gm_status(const char *text);

/// Deletes sprites with ids 0..count-1 from the bottom screen. Every minigame
/// numbers its sprites from 0 so teardown can be shared.
void gm_delete_sprites(int count);

/// Releases graphics slots 0..count-1 in both VRAM and RAM, defragments sprite
/// VRAM, and unloads palette slot 0. Call after gm_delete_sprites().
void gm_free_gfx(int count);

#endif // GAME_H
