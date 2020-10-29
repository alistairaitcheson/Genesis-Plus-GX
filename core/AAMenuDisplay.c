#include "AAMenuDisplay.h"

#include "shared.h"
#include <stdio.h>
#include <sys/types.h>
#include "include/dirent.h"
#include "vdp_render.h"
#include "AAModConsole.h"
#include "AACommonTypes.h"
#include <sys/stat.h>
#include "AALayerRenderer.h"
#include "AACartLoader.h"

static int activeMenu = MENU_LISTING_NONE;

static int chosenGameIndex = 0;
static int optionsItemIndex = 0;

static int majorVersion = 0;
static int minorVersion = 3;

static int DEFAULT_WIDTH = 320;
static int DEFAULT_HEIGHT = 200;

static HackOptions hackOptions;

void menuDisplay_showMenu(int menuNum) {
    activeMenu = menuNum;

    if (activeMenu == MENU_LISTING_TITLE) {
        showTitleMenu();
    }

    if (activeMenu == MENU_LISTING_CHOOSE_GAME) {
        showChooseGameMenu();
    }

    if (activeMenu == MENU_LISTING_SETTINGS) {
        showOptionsMenu();
    }
}

void menuDisplay_hideMenu() {
    activeMenu = MENU_LISTING_NONE;

    layerRenderer_clearLayer(0);
}

void refreshMenu() {
    menuDisplay_showMenu(activeMenu);
}

void beginGame() {
    cartLoader_loadRomAtIndex(chosenGameIndex, 0);
}

int menuDisplay_onButtonPress(int buttonIndex) {
    if (activeMenu == MENU_LISTING_TITLE && buttonIndex == INPUT_INDEX_START) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
        return 1;
    }

    if (activeMenu == MENU_LISTING_CHOOSE_GAME) {
        int romCount = cartLoader_getRomCount();
        if (buttonIndex == INPUT_INDEX_START) {
            menuDisplay_hideMenu();
            beginGame();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_UP) {
            chosenGameIndex--;
            if (chosenGameIndex < 0) {
                chosenGameIndex = romCount - 1;
            }
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            chosenGameIndex++;
            if (chosenGameIndex >= romCount) {
                chosenGameIndex = 0;
            }
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_SETTINGS) {
        if (buttonIndex == INPUT_INDEX_START) {
            menuDisplay_showMenu(MENU_LISTING_CHOOSE_GAME);
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_UP) {
            optionsItemIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            optionsItemIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_LEFT || buttonIndex == INPUT_INDEX_B) {
            incrementOption(-1);
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_RIGHT || buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C) {
            incrementOption(1);
            refreshMenu();
            return 1;
        }
    }

    return 0;
}

void incrementOption(int byAmount) {
    if (optionsItemIndex == 0) {
        hackOptions.infiniteLives += byAmount;
    } else if (optionsItemIndex == 1) {
        hackOptions.infiniteTime += byAmount;
    } else if (optionsItemIndex == 2) {
        hackOptions.copyVram += byAmount;
    }
}

void showTitleMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 1);

    char titleText[0x100];
    sprintf(titleText, "Alistair's Magic Box V%d.%2d", majorVersion, minorVersion);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, titleText, 5);

    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 32, "HOLD (START + UP + A + B)", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 40, "to reset active game", 5);

    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 56, "--- your roms ---", 5);
    for (int i = 0; i < cartLoader_getRomCount() && i < 16; i++)
    {
        char fileNameBuf[0x100];
        cartLoader_getRomFileName(i, fileNameBuf);
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 56 + ((i + 1) * 8), fileNameBuf, 5);
    }  
    if (cartLoader_getRomCount() >= 16) {
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 56 + ((17) * 8), "... and more", 5);
    }

    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to begin ---", 5);
}

void showChooseGameMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 1);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "choose a game", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to select ---", 5);

    int listHeight = 16;
    int halfListHeight = listHeight / 2;
    int romCount = cartLoader_getRomCount();

    int startIndex = chosenGameIndex - halfListHeight;
    int endIndex = chosenGameIndex + halfListHeight;
    if (startIndex < 0) {
        startIndex = 0;
        endIndex = listHeight;
    } else if (endIndex >= romCount) {
        endIndex = romCount - 1;
        startIndex = endIndex - listHeight;
        if (startIndex < 0) {
            startIndex = 0;
        }
    }

    int yPos = 32;
    for (int i = startIndex; i < endIndex; i++) {
        char fileNameBuf[0x100];
        cartLoader_getRomFileName(i, fileNameBuf);
        if (i == chosenGameIndex) {
            char newNameBuf[0x100];
            for (int j = 0; j < 0xF0; j++) {
                newNameBuf[j + 4] = fileNameBuf[j];
            }
            newNameBuf[0] = '>';
            newNameBuf[1] = '>';
            newNameBuf[2] = '>';
            newNameBuf[3] = ' ';
            layerRenderer_writeWord256(0, 16, yPos, newNameBuf, 5);
        } else {
            char newNameBuf[0x100];
            for (int j = 0; j < 0xF0; j++) {
                newNameBuf[j + 3] = fileNameBuf[j];
            }
            newNameBuf[0] = ' ';
            newNameBuf[1] = ' ';
            newNameBuf[2] = ' ';
            layerRenderer_writeWord256(0, 16, yPos, newNameBuf, 5);
        }
        yPos += 8;
    }

}

void showOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 1);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "options", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to play ---", 5);

    int lineCount = 3;
    char lines[lineCount][0x80];

    if (optionsItemIndex < 0) {
        optionsItemIndex = lineCount - 1;
    }
    if (optionsItemIndex >= lineCount) {
        optionsItemIndex = 0;
    }

    if (hackOptions.infiniteLives > 1) {
        hackOptions.infiniteLives = 0;
    }
    if (hackOptions.infiniteLives < 0) {
        hackOptions.infiniteLives = 1;
    }
    if (hackOptions.infiniteLives == 1) {
        sprintf(lines[0], "Infinite lives: ON");
    } else {
        sprintf(lines[0], "Infinite lives: OFF");
    }

    if (hackOptions.infiniteTime > 1) {
        hackOptions.infiniteTime = 0;
    }
    if (hackOptions.infiniteTime < 0) {
        hackOptions.infiniteTime = 1;
    }
    if (hackOptions.infiniteTime == 1) {
        sprintf(lines[1], "Infinite time: ON");
    } else {
        sprintf(lines[1], "Infinite time: OFF");
    }

    if (hackOptions.copyVram > 3) {
        hackOptions.copyVram = 0;
    }
    if (hackOptions.copyVram < 0) {
        hackOptions.copyVram = 3;
    }
    if (hackOptions.copyVram == 0) {
        sprintf(lines[2], "Keep vram on switch: NEVER");
    } else if (hackOptions.copyVram == 1) {
        sprintf(lines[2], "Keep vram on switch: ALWAYS");
    } else if (hackOptions.copyVram == 2) {
        sprintf(lines[2], "Keep vram on switch: SOMETIMES");
    } else if (hackOptions.copyVram == 3) {
        sprintf(lines[2], "Keep vram on switch: KEEP 1%%");
    }

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        char toPrint[0x100];
        if (i == optionsItemIndex) {
            sprintf(toPrint, ">>> %s", lines[i]);
        } else {
            sprintf(toPrint, "    %s", lines[i]);
        }
        layerRenderer_writeWord256(0, 16, yPos, toPrint, 5);

        yPos += 8;
    }
}