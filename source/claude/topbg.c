// SPDX-License-Identifier: CC0-1.0
//
// The top-screen backdrop.
//
// Loaded by hand rather than through NFLib. The text console is not an NFLib
// background -- consoleInit() claimed VRAM_A directly -- and NFLib's allocator
// has no idea it is there, so letting NFLib hand out blocks on this screen
// would eventually land on top of the console's font or its map. Placing this
// background at fixed bases keeps the two out of each other's way.
//
// VRAM_A is 128 KB at 0x06000000:
//
//   0x00000..        console font tiles   (consoleInit tile base 0)
//   0x0F000..0x0F7FF this background's map        (map base 30)
//   0x0F800..0x0FFFF console map          (consoleInit map base 31)
//   0x10000..0x1BAFF this background's tiles      (tile base 4)

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <nds.h>

#include "topbg.h"

#define BG_LAYER 2
#define TILE_BASE 4 // x 16 KB
#define MAP_BASE 30 // x 2 KB

#define VRAM_A_BASE 0x06000000
#define EXT_PAL_BASE 0x06880000

// Behind the portrait. A sprite beats a background at equal priority, so the
// girl (also 3) stays in front, and the console at priority 0 stays in front of
// both.
#define BG_PRIO 3

static bool load_to(const char *path, void *dest, size_t limit) {
  FILE *f = fopen(path, "rb");
  if (f == NULL)
    return false;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  if (size <= 0 || (size_t)size > limit) {
    fclose(f);
    return false;
  }

  // VRAM rejects 8-bit writes and fread does them, so the file lands in main
  // RAM first and is DMA'd across in words.
  void *buf = malloc(size);
  if (buf == NULL) {
    fclose(f);
    return false;
  }

  bool ok = fread(buf, 1, size, f) == (size_t)size;
  fclose(f);

  if (ok)
    dmaCopy(buf, dest, size);
  free(buf);
  return ok;
}

bool topBgInit(void) {
  if (!load_to("nitro:/bg/menu_bg.img",
               (void *)(VRAM_A_BASE + TILE_BASE * 16384), 16384 * 4))
    return false;

  if (!load_to("nitro:/bg/menu_bg.map",
               (void *)(VRAM_A_BASE + MAP_BASE * 2048), 2048))
    return false;

  // The palette goes to an *extended* palette slot rather than BG_PALETTE.
  // Extended palettes only apply to 256-colour backgrounds, so the console --
  // which is 4bpp -- carries on reading the standard palette and its font
  // colours are left exactly as they were. Writing BG_PALETTE instead would
  // recolour the text.
  vramSetBankE(VRAM_E_LCD); // let the CPU see VRAM_E
  bool ok = load_to("nitro:/bg/menu_bg.pal",
                    (void *)(EXT_PAL_BASE + (BG_LAYER << 13)), 512);
  vramSetBankE(VRAM_E_BG_EXT_PALETTE);
  if (!ok)
    return false;

  REG_DISPCNT |= DISPLAY_BG_EXT_PALETTE;

  REG_BG2CNT = BG_COLOR_256 | BG_32x32 | BG_TILE_BASE(TILE_BASE) |
               BG_MAP_BASE(MAP_BASE) | BG_PRIORITY(BG_PRIO);

  REG_DISPCNT |= DISPLAY_BG2_ACTIVE;
  return true;
}
