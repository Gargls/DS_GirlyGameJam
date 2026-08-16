// SPDX-License-Identifier: CC0-1.0
//
// A WarioWare-style timer bar across the bottom of the touch screen. It drains
// left as the round clock runs down. Built once and reused by every minigame.

#ifndef CLAUDE_TIMERBAR_H
#define CLAUDE_TIMERBAR_H

/// Build the bar (hidden). Call once, after NF_InitSpriteSys(1) and before any
/// minigame runs.
void timerBarInit(void);

/// Show the bar filled to left/total of its length. Clamped to [0, full].
void timerBarSet(int left, int total);

/// Hide the whole bar -- for menus, cutscenes, and the result hold.
void timerBarHide(void);

#endif // CLAUDE_TIMERBAR_H
