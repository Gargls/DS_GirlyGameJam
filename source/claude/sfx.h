// SPDX-License-Identifier: CC0-1.0
//
// The girl's two one-shot reaction sounds.

#ifndef CLAUDE_SFX_H
#define CLAUDE_SFX_H

/// Loads the effects from the soundbank. Call once, after musicInit().
void sfxInit(void);

/// A little laugh. Play it whenever her mood becomes GIRL_HAPPY.
void sfxHappy(void);

/// A disappointed "huh". Play it whenever her mood becomes GIRL_DISAPPOINTED.
void sfxDisappointed(void);

#endif // CLAUDE_SFX_H
