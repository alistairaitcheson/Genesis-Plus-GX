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

void menuDisplay_showMenu(int menuNum) {
    activeMenu = menuNum;

    if (activeMenu == MENU_LISTING_TITLE) {
        showTitleMenu();
    }

    if (activeMenu == MENU_LISTING_CHOOSE_GAME) {
        showChooseGameMenu();
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
        menuDisplay_showMenu(MENU_LISTING_CHOOSE_GAME);
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

    return 0;
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
            newNameBuf[0] = ' ';
            newNameBuf[1] = '>';
            newNameBuf[2] = '>';
            newNameBuf[3] = ' ';
            layerRenderer_writeWord256(0, 16, yPos, newNameBuf, 5);
        } else {
            layerRenderer_writeWord256(0, 16, yPos, fileNameBuf, 5);
        }
        yPos += 8;
    }

}