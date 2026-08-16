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

/// Offsets far enough to park her completely off one side of the screen. Used
/// as the endpoints of the cutscene slide.
#define GIRL_OFF_LEFT (-224)
#define GIRL_OFF_RIGHT (224)

/// Shifts all nine cells relative to their home position. (0, 0) puts her back
/// where she belongs; anything else slides her without disturbing the grid.
void girlSetOffset(int dx, int dy);

#endif // CLAUDE_GIRL_H
