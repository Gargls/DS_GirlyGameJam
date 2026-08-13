// SPDX-License-Identifier: CC0-1.0
//
// The stripes wipe on the bottom screen.
//
// Both directions sweep left to right: filling in covers the screen, clearing
// out uncovers it. Nothing here scrolls the stripes into view -- a tiled
// background wraps around, so there is no edge to slide in. The moving edge is
// a hardware window instead.

#ifndef CLAUDE_WIPE_H
#define CLAUDE_WIPE_H

#include <stdbool.h>

/// Tells the wipe which bottom-screen background layer holds the stripes.
/// Call once, after the background has been created.
void wipeInit(int layer);

/// covering = true  -> stripes sweep in from the left until the screen is full.
/// covering = false -> stripes clear away from the left, revealing what is
///                     behind them.
/// Safe to call from any state: it sets the starting point itself.
void wipeStart(bool covering);

/// True while a sweep is in progress. The caller should hold off on input and
/// on running the minigame until this goes false.
bool wipeIsRunning(void);

/// Advances the sweep one frame. Does nothing if none is running.
void wipeUpdate(void);

/// Stops immediately and puts the hardware back to normal. For bailing out to
/// the menu.
void wipeCancel(void);

#endif // CLAUDE_WIPE_H
