// SPDX-License-Identifier: CC0-1.0
//
// Every line of dialogue in the game, in one file, so the writing can be
// rewritten without touching a single piece of logic.
//
// ALL PLACEHOLDER. Replace the strings freely. Each array is terminated by a
// NULL, which is how the cutscene player knows where a scene ends -- so adding
// or removing a line means editing exactly one place and no counter can drift
// out of sync with it.

#ifndef GAME_SCRIPT_H
#define GAME_SCRIPT_H

extern const char *const script_stage1[];
extern const char *const script_stage2[];
extern const char *const script_stage3[];
extern const char *const script_stage4[];
extern const char *const script_fail[];
extern const char *const script_ending[];

#endif // GAME_SCRIPT_H
