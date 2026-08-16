// SPDX-License-Identifier: CC0-1.0
//
// All text in the game. It lives on the *bottom* screen now, next to the thumb
// that advances it, which frees the top screen to be nothing but the portrait.

#ifndef GAME_DIALOGUE_H
#define GAME_DIALOGUE_H

#include <stdbool.h>

/// Rows the rest of the game may not draw sprites over. The play area is
/// everything between them.
#define DLG_HUD_ROW 0
#define DLG_HINT_ROW 2

/// Builds the bottom-screen console. Call once, immediately after
/// NF_InitTiledBgSys(1) and before any NF_LoadTiledBg().
void dialogueInit(void);

/// Wipes the whole screen.
void dialogueClear(void);

/// Blanks one row without disturbing the rest.
void dialogueClearRow(int row);

/// Top line: which stage this is, and how many lives are left. Pass lives < 0
/// to leave the life counter off (menus, endings).
void dialogueHud(const char *stage, int lives);

/// The one-line instruction under the header.
void dialogueHint(const char *text);

/// Draws one page of dialogue in the lower half, word-wrapped, with a prompt
/// telling the player how to continue. Clears the whole box first, so pages do
/// not bleed into each other.
void dialoguePage(const char *text, bool more);

/// Clears just the dialogue box, leaving the header alone.
void dialogueClearBox(void);

/// A centred line, for menu and ending screens.
void dialogueCentred(int row, const char *text);

#endif // GAME_DIALOGUE_H
