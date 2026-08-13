# source/claude/

Written by Claude on 2026-08-13, for tomorrow's recap. Two small modules plus a
handful of marked edits in `source/main.c`.

## girl.c — the top-screen portrait

A DS sprite is **at most 64×64**. The art is used at its native 192×192, cut
into a **3×3 grid of nine sprites**. Nothing is scaled.

She therefore fills the whole screen height, and the text is drawn **over** her.
That works because of priority: on the DS a sprite beats a background at the
*same* priority, so the console is pushed to priority 0 (`bgSetPriority()` in
`main.c`) and her sprites to 3. Lower number wins.

Each of the nine holds all three moods as **animation frames**. NFLib reads a
sprite `.img` as N frames of whatever size you pass to `NF_LoadSpriteGfx()` — so
`girl_q0.img` is 12 KB, which is three 64×64 frames, and the size passed is
`64, 64`, not the file's `64, 192`. Changing mood is then nine
`NF_SpriteFrame()` calls: no reloading, no file access.

Cost: nine cells × three moods × 4 KB = **108 KB of the 128 KB** top-screen
sprite arena. It fits because nothing else uses top-screen sprites.

The one real trap: **sprite RAM slots are a global pool shared by both screens.**
`NF_SPR256GFX[]` is indexed by id alone, with no screen dimension. The saw
minigame holds graphics slots 0–1 and palette slot 0, so the portrait starts at
graphics slot 4 and palette slot 1. Reusing a loaded slot makes NFLib halt the
ROM. VRAM slots and sprite ids *are* per screen, so those can start at 0.

Top-screen sprites live in VRAM_B with palettes in VRAM_F; your text console is
a background in VRAM_A. No collision. `NF_InitSpriteSys(0, 128)` only ORs the
sprite-enable bit into `REG_DISPCNT` — it does not touch the video mode, so the
console survives it.

## round.c — the 5-second clock

Two phases. `ROUND_PLAYING` ticks the clock down and lets the minigame win by
reporting so. When it ends — won or timed out — the girl reacts and it holds on
`ROUND_RESULT` for 1.5 s, because otherwise the reaction would be wiped by the
next screen before it could be read. Then `ROUND_DONE` lets the campaign move on.

`roundIsPlaying()` exists so the main loop can stop calling `game_update()` once
the round is over. Without it a late drag would still move a saw while the
reaction is on screen.

## topbg.c — the top-screen backdrop

`bg/menu_bg` sits behind everything on the top screen and never changes.

Loaded **by hand rather than through NFLib**, on purpose. Your text console is
not an NFLib background — `consoleInit()` claimed VRAM_A directly, at tile base
0 and map base 31 — and NFLib's allocator has no idea it is there. Calling
`NF_InitTiledBgSys(0)` would start handing out blocks that the console's font
and map already occupy. It would look like corrupt artwork rather than an
allocator collision, which is a horrible thing to debug.

So the bases are fixed by hand instead. VRAM_A, 128 KB at `0x06000000`:

```
0x00000..        console font tiles   (consoleInit tile base 0)
0x0F000..0x0F7FF backdrop map                (map base 30)
0x0F800..0x0FFFF console map          (consoleInit map base 31)
0x10000..0x1BAFF backdrop tiles              (tile base 4)
```

Verified as non-overlapping and inside the bank.

The palette is the other half of the problem. The backdrop is 256-colour, so
writing it to `BG_PALETTE` would land on the console's font colours and recolour
your text. It goes to an **extended palette slot** instead — extended palettes
apply only to 256-colour backgrounds, so the 4bpp console carries on reading the
standard palette, untouched.

Layering on the top screen is now, back to front: backdrop (BG2, priority 3) →
girl (sprites, priority 3, which wins the tie) → text (BG3, priority 0).

Files are read into main RAM and DMA'd across, because **VRAM rejects 8-bit
writes** and `fread` does them.

## wipe.c — the stripes sweep

Entering a cutscene fills the screen left to right; entering the minigame clears
it away, also left to right.

**You cannot get this by scrolling the background.** A tiled background wraps
around, and the image is stripes edge to edge, so there is no edge to slide in —
scrolling just moves the pattern. The moving edge is a **hardware window**: the
DS can restrict a layer to a rectangle, and growing that rectangle from the left
is the entire effect. `NF_ScrollBg` is still used, but only to make the pattern
crawl while the sweep happens.

`WININ`/`WINOUT` carry one bit per background layer **plus a bit for sprites**.
Gating sprites through the window too is what lets the stripes hide the saws
without touching a single sprite priority — otherwise every saw would poke
through.

Both directions use the same left-anchored rectangle growing 0 → 256; only the
inside/outside meanings swap. That is what makes it clear away *to the left*
rather than retreating back the way it came.

Two edge cases the hardware forces, both handled in `apply()`:

- **A zero-width window is not expressible** — `X0 == X1` reads as undefined,
  not as empty.
- **Nor is a full-width one**, since the right edge is only 8 bits and 256 does
  not fit.

Both are exactly where a wipe starts and ends, so `apply()` special-cases
`w <= 0` and `w >= 256` by setting `WININ` and `WINOUT` to the *same* value and
ignoring the bounds.

The clock is armed in `step_enter()` but does not tick during a sweep, because
the loop skips `roundUpdate()` while `wipeIsRunning()`. You do not lose time
while the screen is still being handed over.

## Edits in source/main.c

- `#include "claude/girl.h"` / `"claude/round.h"`, and `ROUND_SECONDS`
- `step_enter()` starts a round for minigames, sets her back to idle for cutscenes
- the minigame branch of the loop now drives `roundUpdate()` instead of
  advancing straight off `game_update()`
- `girlInit()` / `girlShow(true)` during setup
- `NF_SpriteOamSet(0)` and `oamUpdate(&oamMain)` in the loop — the top screen has
  its own OAM and needs its own flush
- `bgSetPriority(top.bgId, 0)` right after `consoleInit`, so the text sits in
  front of the full-screen portrait
- **text rows moved to 0–2 and 21–22.** Rows 0–2 are above her head; 21–22 sit
  over her dark coat, where white text reads well. The middle of the screen is
  her face — printing there is legal but hard to read.

## Assets

`tools/gen_girl.py` → nine `girl_q*.bmp` strips, then `tools/bmp2nflib.py`.

Using the art at native size removed a whole class of problem. The source is
keyed on **magenta rather than real alpha** (checked: alpha is 255 everywhere),
so an earlier 128×128 version had the resampler averaging magenta into every
edge — a bright pink fringe around the entire character. At 1:1 there is no
resampling, so there is no fringe and no quantisation either: the palette is the
original 27 colours.

If you ever do want her scaled, the fix is to bleed the artwork outwards into
the keyed region first, scale a separate mask, and re-threshold it to a hard
edge. Scaling the keyed image directly will always fringe.

## Open question for tomorrow

**Failing currently still advances the campaign.** You did not say what should
happen on a timeout — retry the step, lose a life, or carry on. Carrying on was
the least surprising default, and it is one line in the loop to change.
