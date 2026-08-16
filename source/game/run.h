// SPDX-License-Identifier: CC0-1.0
//
// The campaign driver: menu, stages, lives, the fail/restart loop, and the
// ending. main.c does nothing but set the hardware up and call runUpdate().

#ifndef GAME_RUN_H
#define GAME_RUN_H

/// Puts the game on the title screen. Call once, after everything is loaded.
void runInit(void);

/// One frame of whatever the game is currently doing.
void runUpdate(void);

#endif // GAME_RUN_H
