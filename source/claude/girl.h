// SPDX-License-Identifier: CC0-1.0
//
// The Popular Girl portrait on the top screen. She reacts to how a round went.

#ifndef CLAUDE_GIRL_H
#define CLAUDE_GIRL_H

#include <stdbool.h>

typedef enum { GIRL_IDLE, GIRL_HAPPY, GIRL_DISAPPOINTED } GirlMood;

/// Loads the portrait and puts it on the top screen. Call once, after
/// NF_SetRootFolder() and NF_InitSpriteBuffers().
void girlInit(void);

/// Swaps which mood is drawn. Cheap -- every frame is already in VRAM, so this
/// only repoints the sprites.
void girlSetMood(GirlMood mood);

void girlShow(bool visible);

#endif // CLAUDE_GIRL_H
