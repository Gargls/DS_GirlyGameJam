// SPDX-License-Identifier: CC0-1.0
//
// Every minigame handle in one place, so campaign.c needs a single include.
//
// Each game lives in its own file with everything static except the handle
// itself -- the implementation cannot be reached by accident from outside.
//
// RESOURCE BUDGET. Get this wrong and NFLib halts the ROM with error 109,
// "already in use", the first time the offending game loads.
//
// There are two different pools and they behave differently:
//
//   RAM slots       NF_SPR256GFX[] / NF_SPR256PAL[] -- ONE dimension, no screen
//                   index. GLOBAL across both screens. A top-screen sprite and
//                   a bottom-screen sprite genuinely collide here.
//   ext pal slots   NF_SPR256VRAM[2][128] -- per screen, so screen 1 may reuse
//                   a number the girl holds on screen 0.
//
// Permanently occupied RAM slots:
//
//   girl (screen 0)   graphics 4-12, palette 1
//   timer bar         graphics 2,    palette 2
//
// So a minigame may use:
//
//   graphics RAM   0, 1, 3
//   palette RAM    0, 3, 4, ...
//   ext palette    0, 1, 2      (the timer bar holds 3 on screen 1)
//   sprite ids     0-63         (the timer bar holds 100-115)

#ifndef MG_GAMES_H
#define MG_GAMES_H

#include "../game/minigame.h"

// Stand-in used wherever the real game is still waiting on art.
extern const Minigame placeholderMinigame;

// Stage 2 -- real mechanics, wrong theme for now.
extern const Minigame sawMinigame;
extern const Minigame heartsMinigame;
extern const Minigame starsMinigame;

// Stage 3 -- The Deed. Placeholder art, real mechanics.
extern const Minigame bopMinigame;
extern const Minigame cutMinigame;
extern const Minigame cleanMinigame;

#endif // MG_GAMES_H
