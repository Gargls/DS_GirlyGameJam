// SPDX-License-Identifier: CC0-1.0
//
// Sets the hardware up and runs the loop. Everything about the *game* lives in
// source/game/ (the campaign, cutscenes, lives) and source/mg/ (the minigames).
//
// Screen layout, which is what most of this file is arranging:
//
//   TOP    (main engine)  backdrop on BG2 (per stage), the girl as nine
//                         sprites, no text
//   BOTTOM (sub engine)   console on BG0, shop backdrop on BG1 (stage 2
//                         minigames only), stripes on BG3, minigame sprites
//
// The text used to be on the top screen and had to dodge her face -- rows 0-2
// above her head, rows 21-22 over her coat, nothing in between. Moving it to
// the touch screen removes that constraint entirely and puts the dialogue next
// to the thumb that advances it.

#include <stdio.h>

#include <filesystem.h>
#include <nds.h>

#include <nf_lib.h>

#include "claude/girl.h"
#include "claude/music.h"
#include "claude/timerbar.h"
#include "claude/topbg.h"
#include "claude/wipe.h"
#include "game/dialogue.h"
#include "game/run.h"

// Backmost of the four layers, which is what a backdrop wants, and it leaves
// 0-2 free -- the console takes 0.
#define BG_STRIPES_LAYER 3

// Decorative backdrop behind stage 2's minigames -- a shop counter, standing
// in for a coin/pay minigame that does not exist yet. Layer 1 sits behind the
// sprites (layer 0, NFLib's default) and in front of the stripes; run.c shows
// and hides it per stage.
#define BG_SHOP_LAYER 1

int main(int argc, char **argv) {
  // The top screen is the main engine. Its backdrop is loaded by hand into
  // VRAM_A rather than through NFLib -- see claude/topbg.c for why.
  videoSetMode(MODE_0_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  lcdMainOnTop();

  // NitroFS must be mounted before NFLib reads any file.
  if (!nitroFSInit(NULL)) {
    perror("nitroFSInit()");
    while (1)
      swiWaitForVBlank();
  }
  NF_SetRootFolder("NITROFS");

  musicInit();

  // Bottom screen. NF_Set2D() goes first: it clears the enable bits that the
  // init calls below set.
  NF_Set2D(1, 0);

  NF_InitTiledBgBuffers();
  NF_InitTiledBgSys(1);

  // Immediately after NF_InitTiledBgSys(1) and before anything is loaded --
  // dialogueInit() shrinks NFLib's VRAM pools to carve out room for the
  // console, and NFLib would otherwise have already handed those blocks away.
  dialogueInit();

  NF_InitSpriteBuffers();
  NF_InitSpriteSys(1);

  NF_LoadTiledBg("bg/stripes_bg", "stripes", 256, 256);
  NF_CreateTiledBg(1, BG_STRIPES_LAYER, "stripes");
  NF_HideBg(1, BG_STRIPES_LAYER);
  wipeInit(BG_STRIPES_LAYER);

  NF_LoadTiledBg("bg/shop_bg", "shop", 256, 256);
  NF_CreateTiledBg(1, BG_SHOP_LAYER, "shop");
  NF_HideBg(1, BG_SHOP_LAYER);

  // Top-screen backdrop, behind the portrait. run.c swaps it per stage.
  topBgLoad("menu_bg");

  // The portrait. girlInit() sets up screen 0's sprite engine itself.
  girlInit();
  girlShow(true);

  // Persistent, on slots no minigame touches, so loading a game never disturbs
  // it. Must come after NF_InitSpriteSys(1).
  timerBarInit();

  runInit();

  while (1) {
    scanKeys();

    musicUpdate();
    runUpdate();

    // Each screen has its own OAM and needs its own flush.
    NF_SpriteOamSet(0);
    NF_SpriteOamSet(1);
    swiWaitForVBlank();
    oamUpdate(&oamMain);
    oamUpdate(&oamSub);
  }

  return 0;
}
