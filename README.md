# Sawmill Party

A Nintendo DS microgame collection in the WarioWare mould: a menu, then three
touch-screen jobs back to back.

| # | Job | Mechanic |
|---|-----|----------|
| 1 | Pay the right amount | Drag coins from the pile into the hand until they total the price |
| 2 | Saw the logs | Drag the saw back and forth across each cut mark |
| 3 | Hammer the nails | Tap each nail three times to drive it flush |

Each one is deliberately a different verb — drag-and-drop, drag-to-scrub, and
tap-to-repeat — so they do not blur together.

### Dealing a solvable coin round

The coin game deals 6–9 coins of 1, 2 and 5 into a loose pile, then sets the
price to **the sum of a random subset of the coins it just dealt**. Solvability
is therefore structural: the subset that produced the price is itself a valid
answer, and there is no search that could fail or need retrying.

The subset is between two coins and all-but-one, so a round is never a single
coin and never just tipping the whole pile in.

Two constants shape the pile. `PILE_MIN_GAP` is the minimum spacing, enforced by
rejection sampling — it exists because **the value is printed in the middle of
the coin**, so coins that overlap much past it lose their digit and the player
ends up guessing at what is in the pile. `PILE_TRIES` bounds the retries; a coin
that cannot find room is placed anyway, which only means a tighter heap.

`rand()` is seeded from a frame counter at the moment the player first touches
the menu — the only free entropy on the DS here.

## Build and run

```sh
make                          # builds GameJam_DS.nds
bash tools/convert_assets.sh  # only needed after changing art
```

The toolchain is **BlocksDS** at `/opt/blocksds`, not devkitPro. A devkitPro
install also exists on this machine and `DEVKITPRO` is set, so if a header or
tool ever resolves oddly, check which one you actually picked up.

`make` builds from whatever is already in `nitrofiles/`. Art conversion is a
separate step on purpose, so a normal code build stays fast.

## Layout

```
source/         engine + one file per minigame
  main.c          init, campaign flow, shared helpers
  game.h          the Minigame interface
  mg_*.c          the minigames
tools/          asset generators and converters (Python + one shell script)
Graphics/       source art
  generated/      procedurally drawn art -- regenerated, do not hand-edit
nitrofiles/     converted assets; this whole tree is packed into the ROM
attic/          unused files kept out of the build, safe to delete
```

## How the screens are wired

The **top screen** is a text layer over a fixed backdrop. It never changes
structure, only its text.

The **bottom screen** is the play field: it swaps its background per minigame
and owns every sprite.

VRAM splits cleanly, which is why all three subsystems coexist:

| Bank | Use |
|------|-----|
| A, E | top screen background + extended palettes |
| C, H | bottom screen background + extended palettes |
| D, I | bottom screen sprites + extended palettes |

NFLib uses **extended background palettes**, so each layer gets its own 256
colours. That is what lets a 256-colour photographic background and the font
share one screen without fighting over palette entries.

## The between-games wipe

Every change of state on the bottom screen goes through a diagonal barber-pole
wipe: it fills in from the left, everything swaps over behind it, then it clears
away to the left again.

The pattern is a **normal tiled background on layer 2** (`stripes_bg`), created
once at startup and hidden except during a transition. It is periodic in both
axes at 32px, so grit collapses the whole 256×256 image to **5 unique tiles**,
and the crawl is just `NF_ScrollBg()` in X — scrolling by `dx` shifts the
pattern by exactly `dx`.

The reveal is a **hardware window** (`WIN0` on the sub engine). `WININ`/`WINOUT`
carry one bit per background layer *plus a bit for sprites*, so the window is
told to show the stripe layer inside and the scene-plus-sprites outside. Using
it to gate sprites as well is what makes this work **without touching a single
sprite priority** — otherwise every minigame's sprites would poke through the
stripes.

Both halves use the same left-anchored rectangle growing 0 → 256; only the
inside/outside meanings swap. That is what makes it clear away to the left
rather than retreating back the way it came.

Two edge cases the hardware forces:

- **A zero-width window is not expressible** — `X0 == X1` reads as undefined,
  not as empty.
- **A full-width one is not either**, since the right edge is only 8 bits and
  256 does not fit.

So `wipe_apply()` handles `w <= 0` and `w >= 256` by setting `WININ` and
`WINOUT` to the *same* value and ignoring the bounds entirely.

The swap happens at the midpoint, fully covered: the old minigame tears down,
the background changes, the next one builds. `update()` is not called during a
wipe, so the incoming game is frozen while it is revealed.

The effect is ported from `drawStripes()` in the TOUCH-WARE project
(`~/git/DS_Game/CLAUDE-DS-Game/source/main.c`), which software-renders it per
scanline into a bitmap. None of that code transfers — this ROM has no
framebuffer — but the shape is the same one, re-expressed as
`((x - y) mod 32) < 16`.

## Adding a minigame

1. Write `source/mg_yours.c` exposing `const Minigame mg_yours`.
2. Declare it in `source/game.h`.
3. Add `&mg_yours` to the `campaign[]` array in `source/main.c`.

The Makefile globs `source/` recursively, so there is nothing to register in
the build.

The contract is in `game.h`: `start()` loads and creates, `update()` runs one
frame and returns `true` once the goal is met, `stop()` tears down. Number your
sprites from 0 and your graphics slots from 0, then `gm_delete_sprites()` and
`gm_free_gfx()` handle teardown for you.

Two rules worth internalising:

**Settle the win on release, not mid-interaction.** The engine stops calling
`update()` the moment it returns `true`, so anything still animating freezes
where it stands. `mg_coins` only checks the total when the pen lifts, and
`mg_saw` waits for the cut halves to fall off-screen before it reports a win.

**One state transition per frame.** The engine's `switch` handles exactly one
state per iteration, so the tap that starts a minigame is never also seen by
that minigame. Do not work around this in your own code.

## Asset pipeline

Everything in `nitrofiles/` is generated. `tools/convert_assets.sh` rebuilds all
of it in one go: it runs the four generators, then converts sprites with
`bmp2nflib.py` and backgrounds and the font with `grit`.

| Generator | Produces |
|-----------|----------|
| `gen_font.py` | `smallfont.png` — the 5px-cap text font |
| `gen_hand.py` | `hand.bmp` — the open hand for the coin game |
| `gen_props.py` | logs, saw, nail strip, hammer |
| `gen_bg.py` | `bench_bg.png`, `wall_bg.png` — wooden backdrops; `stripes_bg.png` — the wipe pattern |

Art in `Graphics/generated/` is overwritten on every run. Edit the generator,
not the output.

### Format rules that will bite you

**Sprite VRAM is tiled, not linear.** It is a sequence of 8×8 tiles, left to
right then top to bottom. Writing row-major pixel data produces a sprite that
renders as vertical striping — this is not a palette bug, which is what it
looks like. `bmp2nflib.py` reorders for you.

**`.img` files carry no dimensions.** The size you pass to `NF_LoadSpriteGfx()`
is the only thing that defines the shape, and getting it wrong silently
garbles the sprite. `bmp2nflib.py` prints the sizes to pass.

**Index 0 is transparent.** `FF00FF` maps to it. Nothing else may use index 0.

**Tiled backgrounds must be a multiple of 256 pixels on both sides.**
`NF_LoadTiledBg()` hard-errors otherwise. The 256×192 art is padded to 256×256;
the pad sits below the visible area. Backgrounds are also quantised to 255
colours, since index 0 is spent on transparency.

**Anything that gets scrolled must be authored at the full 256×256**, not
256×192 — otherwise the black padding scrolls into view. `convert_assets.sh`
passes 256×256 sources through untouched and only pads the shorter ones.

**Text is an 8×8 tile grid**, 32 columns by 24 rows, and that cannot change.
`NF_WriteText()` maps a character with `tile = char - 32` and silently *wraps
mid-word* once a string runs past the right edge, so keep strings short enough
to fit from wherever you place them. Only ASCII 32–126 is safe; anything higher
is turned into a space.

Because the cell is fixed, a "smaller font" can only mean smaller glyphs inside
it. `smallfont` uses 5px caps and 1px strokes against NFLib's default 6px, and
carries a **1px dark outline** so white text stays readable over a photographic
background.

**Multi-frame sprites stack vertically.** A file of N frames is just the frames
one above another; NFLib derives the count from file size ÷ frame size. Because
the tiler walks the image in 8-row bands, stacking already produces the right
tile order. `nail.img` is 16×128 = four 16×32 frames, and every sprite created
from it keeps its own frame counter, so five nails share one VRAM slot and sink
independently.

## Gotchas already paid for

- **`SPRITE_COUNT` is taken.** libnds defines it as 128 in
  `nds/arm9/sprite.h`. The minigames use `MG_SPRITES`.
- **`NF_Set2D()` must come first.** It calls `videoSetMode*()`, which clears the
  sprite-enable bit that `NF_InitSpriteSys()` sets. The other init calls only OR
  bits into `REG_DISPCNT`, so their order does not matter.
- **`nitroFSInit()` is required.** `NF_SetRootFolder("NITROFS")` only *checks*
  the mount with `access()`; it does not create it. Skipping the init gives you
  file-not-found on every asset.
- **`scanKeys()` exactly once per frame.** It latches edges, so a second call
  makes `keysDown()` return zero.
- **`touchRead()` is only valid while `KEY_TOUCH` is held.** The ADC returns
  garbage on the release frame, which is why every minigame handles release in
  the `else` branch using the last known position.
- **Lower sprite id draws on top.** Backdrop-ish sprites need *higher* ids than
  the things that sit on them.
- **Defragment sprite VRAM after freeing.** `NF_FreeSpriteGfx()` leaves holes;
  without `NF_VramSpriteGfxDefrag()` the next minigame can fail to find one
  contiguous block despite enough free VRAM. `gm_free_gfx()` does this.
- **`iprintf` is not declared under `-std=gnu17`** in this newlib. Use `printf`
  — though the ROM no longer uses either, since text goes through NFLib.

## Art credits

All art in `Graphics/generated/` and the font are drawn by the generators in
`tools/` and carry no third-party licence. The coins are hand-drawn
(`Graphics/coins.aseprite`). The two photographic backgrounds in
`Graphics/Placeholder_Backgrounds/` are placeholders and should be replaced
before any public release — they are also the most expensive assets in the ROM,
at roughly 700 unique tiles each against 400 for the drawn ones.
