// SPDX-License-Identifier: CC0-1.0
//
// PLACEHOLDER DIALOGUE. All of it. Rewrite freely -- nothing outside this file
// knows or cares what the strings say.
//
// Every array ends with NULL. Do not remove that; it is what marks the end of a
// scene. Keep a page under about 180 characters (six lines of thirty) or the
// tail is dropped without warning.

#include <stddef.h>

#include "script.h"

// Stage 1 -- she arrives, idle, and brightens up. First thing the player sees.
const char *const script_stage1[] = {
    "[PLACEHOLDER] Oh! Is it that time already? I have so much to get ready.",
    "[PLACEHOLDER] He is going to be here tonight. Everything has to be perfect.",
    "[PLACEHOLDER] Help me get ready, would you?",
    NULL,
};

// Stage 2 -- shopping. She stays idle throughout.
const char *const script_stage2[] = {
    "[PLACEHOLDER] Now, the shopping. I have a list, and I do not intend to miss a thing on it.",
    "[PLACEHOLDER] Some of these are for dinner. Some of them are not.",
    NULL,
};

// Stage 3 -- she comes back happy, says something breezy, and leaves.
const char *const script_stage3[] = {
    "[PLACEHOLDER] There you are! I did say everything had to be perfect.",
    "[PLACEHOLDER] Do not look at me like that. You knew what this was.",
    "[PLACEHOLDER] I will be in the kitchen. Tidy up for me, would you?",
    NULL,
};

// Stage 4 -- somber, but she is still delighted. That is the joke.
const char *const script_stage4[] = {
    "[PLACEHOLDER] Candles. I did promise candlelight.",
    "[PLACEHOLDER] Dinner is nearly ready. He has never been this quiet.",
    "[PLACEHOLDER] It is going to be a lovely evening.",
    NULL,
};

// Shown when the last life is lost, before the stage restarts.
const char *const script_fail[] = {
    "[PLACEHOLDER] No, no, no. That is not how this goes at all.",
    "[PLACEHOLDER] Again. From the top.",
    NULL,
};

const char *const script_ending[] = {
    "[PLACEHOLDER] And they lived happily ever after. One of them, anyway.",
    NULL,
};
