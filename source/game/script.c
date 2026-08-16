// SPDX-License-Identifier: CC0-1.0
#include <stddef.h>

#include "script.h"

const Line script_stage1[] = {
    {"Oh! Is it that time already? I have so much to get ready.", GIRL_IDLE},
    {"We're going to the mall today and prepare for tonight! Everything has to "
     "be perfect.",
     GIRL_IDLE},
    {"Help me get ready, would you?", GIRL_HAPPY},
    {NULL, GIRL_IDLE},
};

const Line script_stage2[] = {
    {"Now, the shopping. I have a list, and I do not intend to miss a thing on "
     "it.",
     GIRL_IDLE},
    {"Some of these are for dinner. Some of them are not.", GIRL_HAPPY},
    {NULL, GIRL_IDLE},
};

const Line script_stage3[] = {
    {"Hah, I'm so excited. Haven't had a fun day like this in awhile",
     GIRL_DISAPPOINTED},
    {"I am exhausted, now what was on the Plan again?", GIRL_IDLE},
    {"Ah right, I'll go meet up with him now. You know what to do, right?",
     GIRL_HAPPY},
    {NULL, GIRL_IDLE},
};

const Line script_stage4[] = {
    {"Candles? You even brought Candles?", GIRL_HAPPY},
    {"Wow, I didn't expect it to be like this", GIRL_IDLE},
    {"It is going to be a lovely evening.", GIRL_HAPPY},
    {NULL, GIRL_IDLE},
};

const Line script_fail[] = {
    {"No, no, no. That is not how this goes at all.", GIRL_DISAPPOINTED},
    {"Again. From the top.", GIRL_DISAPPOINTED},
    {NULL, GIRL_IDLE},
};

const Line script_ending[] = {
    {"Yumm, this is perfect. You clearly have outdone yourself today!",
     GIRL_HAPPY},
    {NULL, GIRL_IDLE},
};
