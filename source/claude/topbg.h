// SPDX-License-Identifier: CC0-1.0
//
// The menu artwork as a permanent backdrop on the top screen.

#ifndef CLAUDE_TOPBG_H
#define CLAUDE_TOPBG_H

#include <stdbool.h>

/// Puts bg/menu_bg behind everything on the top screen and leaves it there.
/// Call once, after NitroFS is mounted and after consoleInit().
///
/// Returns false if a file could not be read; the rest of the game is
/// unaffected either way.
bool topBgInit(void);

#endif // CLAUDE_TOPBG_H
