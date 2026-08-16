// SPDX-License-Identifier: CC0-1.0
//
// The bottom-screen text console.
//
// Getting a libnds console onto the sub screen takes one piece of care.
// NF_InitTiledBgSys(1) claims *all* 128 KB of the sub background bank -- 96 KB
// of tiles and 32 KB of maps -- and its allocator has no idea a console exists,
// so it will eventually hand out the blocks the font and map are sitting in.
//
// The fix is to shrink its pools. NF_BANKS_TILES / NF_BANKS_MAPS bound the
// allocator's search on every load, so lowering them reserves the tail of each
// pool for us. They cannot be set beforehand -- NF_InitTiledBgSys *hardcodes*
// them back to 8 and 16 (nf_tiledbg.c:92) -- so this has to run after it.
//
// VRAM_C is 128 KB at 0x06200000, in 16 KB tile banks and 2 KB map banks (the
// first two tile banks are what the map banks live in):
//
//   0x00000..0x077FF  NFLib map banks 0-14
//   0x07800..0x07FFF  this console's map     (map base 15)   <- reserved
//   0x08000..0x1BFFF  NFLib tile banks 2-6
//   0x1C000..0x1DFFF  this console's font    (tile base 7)   <- reserved
//   0x1E000..0x1FFFF  spare
//
// The font is 4bpp, 256 tiles x 32 bytes = 8 KB, and the map is 32x32 x 2 bytes
// = 2 KB exactly. Both fit their reserved block.
//
// Colours are safe for the same reason they were on the top screen: NFLib runs
// its backgrounds on *extended* palettes, which a 4bpp console does not read,
// so nothing here can recolour the artwork and nothing there can recolour text.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <nds.h>

#include <nf_lib.h>

#include "dialogue.h"

#define DLG_LAYER 0
#define DLG_MAPBASE 15 // x 2 KB
#define DLG_TILEBASE 7 // x 16 KB

// The console is 32x24 characters. Text is inset by one column so it does not
// touch the bezel.
#define COL0 1
#define COLS 30
#define ROWS 24

// The dialogue box. It starts below the play area and runs to the bottom,
// which is only safe because the timer bar is hidden during cutscenes -- the
// bar's sprites sit at y=180, over rows 22 and 23.
#define BOX_TOP 15
#define BOX_LINES 6
#define BOX_PROMPT_ROW 22

static PrintConsole bottom;

void dialogueInit(void) {
  // See the header comment: after NF_InitTiledBgSys(1), never before.
  NF_BANKS_TILES[1] = DLG_TILEBASE;
  NF_BANKS_MAPS[1] = DLG_MAPBASE;

  consoleInit(&bottom, DLG_LAYER, BgType_Text4bpp, BgSize_T_256x256, DLG_MAPBASE,
              DLG_TILEBASE, false, true);
  consoleSelect(&bottom);
}

void dialogueClear(void) { printf("\x1b[2J"); }

void dialogueClearRow(int row) {
  // 32 spaces is exactly one row; printing them is cheaper than an escape
  // sequence per cell and leaves the cursor somewhere harmless.
  printf("\x1b[%d;0H                                ", row);
}

void dialogueHud(const char *stage, int lives) {
  dialogueClearRow(DLG_HUD_ROW);
  printf("\x1b[%d;%dH%s", DLG_HUD_ROW, COL0, stage);

  if (lives < 0)
    return;

  // Right-aligned, so a changing life count never shifts the stage name.
  // Wide enough for any int the compiler can imagine, so it cannot warn about
  // truncation. In practice lives is a single digit.
  char buf[24];
  snprintf(buf, sizeof(buf), "LIVES %d", lives);
  printf("\x1b[%d;%dH%s", DLG_HUD_ROW, (int)(32 - COL0 - strlen(buf)), buf);
}

void dialogueHint(const char *text) {
  dialogueClearRow(DLG_HINT_ROW);
  if (text != NULL)
    printf("\x1b[%d;%dH%s", DLG_HINT_ROW, COL0, text);
}

void dialogueClearBox(void) {
  for (int r = BOX_TOP; r < ROWS; r++)
    dialogueClearRow(r);
}

void dialogueCentred(int row, const char *text) {
  int len = (int)strlen(text);
  int col = len >= 32 ? 0 : (32 - len) / 2;
  dialogueClearRow(row);
  printf("\x1b[%d;%dH%s", row, col, text);
}

// Greedy word wrap. Breaks on spaces, and falls back to a hard break for a
// single word longer than the line -- otherwise a long word would loop forever.
void dialoguePage(const char *text, bool more) {
  dialogueClearBox();

  int row = BOX_TOP;
  const char *p = text;

  while (*p != '\0' && row < BOX_TOP + BOX_LINES) {
    while (*p == ' ')
      p++;
    if (*p == '\0')
      break;

    int len = (int)strlen(p);
    int take = len < COLS ? len : COLS;

    if (take == COLS) {
      // Back up to the last space that fits, so words stay whole.
      int brk = take;
      while (brk > 0 && p[brk] != ' ' && p[brk] != '\0')
        brk--;
      if (brk > 0)
        take = brk;
    }

    printf("\x1b[%d;%dH%.*s", row, COL0, take, p);
    p += take;
    row++;
  }

  printf("\x1b[%d;%dH%s", BOX_PROMPT_ROW, COL0,
         more ? "TOUCH to continue >>" : "TOUCH to begin >>");
}
