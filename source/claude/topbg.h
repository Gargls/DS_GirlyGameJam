// SPDX-License-Identifier: CC0-1.0
//
// The top-screen backdrop, swappable per stage.

#ifndef CLAUDE_TOPBG_H
#define CLAUDE_TOPBG_H

#include <stdbool.h>

/// Puts nitrofiles/bg/<name>.{img,map,pal} behind everything on the top
/// screen, replacing whatever was there. Call once at startup with the menu
/// background, then again whenever the campaign moves to a new stage.
///
/// Returns false if a file could not be read; the rest of the game is
/// unaffected either way.
bool topBgLoad(const char *name);

#endif // CLAUDE_TOPBG_H
