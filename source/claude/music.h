// SPDX-License-Identifier: CC0-1.0
//
// Looping background music, streamed from NitroFS.

#ifndef CLAUDE_MUSIC_H
#define CLAUDE_MUSIC_H

/// Sets maxmod up for streaming. Call once, after NitroFS is mounted.
void musicInit(void);

/// Streams nitrofiles/audio/<name>.wav on a loop. Calling this with the name
/// already playing does nothing -- so a stage transition can call it every
/// time without restarting the song underneath a scene that keeps the same
/// track.
void musicPlay(const char *name);

/// Reads more of the current file into the streaming buffer. Call every
/// frame, regardless of what else is on screen -- a wipe or a paused round
/// must not starve the music.
void musicUpdate(void);

#endif // CLAUDE_MUSIC_H
