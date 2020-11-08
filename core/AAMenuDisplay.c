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
static int minorVersion = 4;

static int DEFAULT_WIDTH = 320;
static int DEFAULT_HEIGHT = 200;

static HackOptions hackOptions;

HackOptions menuDisplay_getHackOptions() {
    return hackOptions;
}

int menuDisplay_isShowing() {
    if (activeMenu == MENU_LISTING_NONE) {
        return 0;
    } else {
        return 1;
    }
}

void menuDisplay_initialise() {
    // char path[0x100];
    // char folder[0x10];
    // writeFolderPathIntoArray32(folder);
    // sprintf(path, "%s/__prefs.data", folder);
    // FILE *prefsReader = fopen(path, "rb");

    FILE *prefsReader = fopen("_magicbox/__prefs.data", "rb");

    if (prefsReader) {
        int prefsBuffer[0x100];
        fread(prefsBuffer, sizeof(int), 0x100, prefsReader);
        fclose(prefsReader);
        applySettingsFromArray256(prefsBuffer);
    } else {
        applyDefaultSettings();
    }
}

void applySettingsFromArray256(int array256[]) {
    hackOptions.infiniteLives = array256[0];
    hackOptions.infiniteTime = array256[1];
    hackOptions.copyVram = array256[2];
    hackOptions.switchGameType = array256[3];
    hackOptions.cooldownOnSwitch = array256[4];
    hackOptions.speedUpOnRing = array256[5];
    hackOptions.loadFromSavedState = array256[6];
    hackOptions.automaticallySaveStatesFreq = array256[7];
}

void applyDefaultSettings() {
    hackOptions.infiniteTime = 1;
    hackOptions.infiniteLives = 1;
    hackOptions.copyVram = 0;
    hackOptions.switchGameType = 1;
    hackOptions.cooldownOnSwitch = 0;
    hackOptions.speedUpOnRing = 0;
    hackOptions.loadFromSavedState = 0;
    hackOptions.automaticallySaveStatesFreq = 1;

    saveHackOptions();
}

void saveHackOptions() {
    int options[0x100];
    for (int i = 0; i < 0x100; i++) {
        options[i] = 0;
    }
    options[0] = hackOptions.infiniteLives;
    options[1] = hackOptions.infiniteTime;
    options[2] = hackOptions.copyVram;
    options[3] = hackOptions.switchGameType;
    options[4] = hackOptions.cooldownOnSwitch;
    options[5] = hackOptions.speedUpOnRing;
    options[6] = hackOptions.loadFromSavedState;
    options[7] = hackOptions.automaticallySaveStatesFreq;

    // char path[0x100];
    // char folder[0x10];
    // writeFolderPathIntoArray32(folder);
    // sprintf(path, "%s/__prefs.data", folder);
    remove("_magicbox/__prefs.data");
    FILE *prefsWriter = fopen("_magicbox/__prefs.data", "wb");

    for (int i = 0; i < 0x100; i++) {
        fwrite(options, sizeof(int), 0x100, prefsWriter);
    }
    fclose(prefsWriter);
}

void menuDisplay_showMenu(int menuNum) {
    activeMenu = menuNum;
    vdp_setShouldRandomiseColours(1);
    aa_psg_mute();
    aa_ym2612_mute();

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
    vdp_setShouldRandomiseColours(0);
    aa_psg_unmute();
    aa_ym2612_unmute();
    cartLoader_applyHackOptions();
    modConsole_applyHackOptions();
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
            saveHackOptions();
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
        hackOptions.switchGameType += byAmount;
    } else if (optionsItemIndex == 1) {
        hackOptions.cooldownOnSwitch += byAmount;
    } else if (optionsItemIndex == 2) {
        hackOptions.copyVram += byAmount;
    } else if (optionsItemIndex == 3) {
        hackOptions.speedUpOnRing += byAmount;
    } else if (optionsItemIndex == 4) {
        hackOptions.infiniteLives += byAmount;
    } else if (optionsItemIndex == 5) {
        hackOptions.infiniteTime += byAmount;
    } else if (optionsItemIndex == 6) {
        hackOptions.loadFromSavedState += byAmount;
    } else if (optionsItemIndex == 7) {
        hackOptions.automaticallySaveStatesFreq += byAmount;
    }
}

void showTitleMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);

    char titleText[0x100];
    sprintf(titleText, "Alistair's Magic Box V%d.%02d", majorVersion, minorVersion);
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

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
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
                newNameBuf[j + 3] = fileNameBuf[j];
            }
            newNameBuf[0] = '>';
            newNameBuf[1] = '>';
            newNameBuf[2] = ' ';
            layerRenderer_writeWord256WithBorder(0, 16, yPos, newNameBuf, 5, 1, 0);
        } else {
            char newNameBuf[0x100];
            for (int j = 0; j < 0xF0; j++) {
                newNameBuf[j + 3] = fileNameBuf[j];
            }
            newNameBuf[0] = ' ';
            newNameBuf[1] = ' ';
            newNameBuf[2] = ' ';
            layerRenderer_writeWord256WithBorder(0, 16, yPos, newNameBuf, 5, 1, 0);
        }
        yPos += 8;
    }

}

void showOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "options", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to play ---", 5);

    int lineCount = 8;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
    }

    if (optionsItemIndex < 0) {
        optionsItemIndex = lineCount - 1;
    }
    if (optionsItemIndex >= lineCount) {
        optionsItemIndex = 0;
    }

    if (hackOptions.switchGameType > 4) {
        hackOptions.switchGameType = 0;
    }
    if (hackOptions.switchGameType < 0) {
        hackOptions.switchGameType = 4;
    }
    if (hackOptions.switchGameType == 0) {
        sprintf(lines[0], "Switch games:            OFF");
        blockedLines[1] = 1;
        blockedLines[2] = 1;
    } else if (hackOptions.switchGameType == 1) {
        sprintf(lines[0], "Switch games:    ON GET RING");
    } else if (hackOptions.switchGameType == 2) {
        sprintf(lines[0], "Switch games:   EVERY 5 secs");
    } else if (hackOptions.switchGameType == 3) {
        sprintf(lines[0], "Switch games:  EVERY 10 secs");
    } else if (hackOptions.switchGameType == 4) {
        sprintf(lines[0], "Switch games:  EVERY 30 secs");
    }

    if (hackOptions.cooldownOnSwitch > 1) {
        hackOptions.cooldownOnSwitch = 0;
    }
    if (hackOptions.cooldownOnSwitch < 0) {
        hackOptions.cooldownOnSwitch = 1;
    }
    if (hackOptions.cooldownOnSwitch == 0) {
        sprintf(lines[1], "Cooldown after switch:   OFF");
    } else {
        sprintf(lines[1], "Cooldown after switch: 1 sec");
    }

    if (hackOptions.copyVram > 4) {
        hackOptions.copyVram = 0;
    }
    if (hackOptions.copyVram < 0) {
        hackOptions.copyVram = 3;
    }
    if (hackOptions.copyVram == 0) {
        sprintf(lines[2], "Keep vram on switch:     OFF");
    } else if (hackOptions.copyVram == 1) {
        sprintf(lines[2], "Keep vram on switch:   100%%");
    } else if (hackOptions.copyVram == 2) {
        sprintf(lines[2], "Keep vram on switch:    50%%");
    } else if (hackOptions.copyVram == 3) {
        sprintf(lines[2], "Keep vram on switch:    10%%");
    } else if (hackOptions.copyVram == 4) {
        sprintf(lines[2], "Keep vram on switch:     1%%");
    }

    if (hackOptions.speedUpOnRing > 1) {
        hackOptions.speedUpOnRing = 0;
    }
    if (hackOptions.speedUpOnRing < 0) {
        hackOptions.speedUpOnRing = 1;
    }
    if (hackOptions.speedUpOnRing == 1) {
        sprintf(lines[3], "Speed up on ring:         ON");
    } else {
        sprintf(lines[3], "Speed up on ring:        OFF");
    }



    if (hackOptions.infiniteLives > 1) {
        hackOptions.infiniteLives = 0;
    }
    if (hackOptions.infiniteLives < 0) {
        hackOptions.infiniteLives = 1;
    }
    if (hackOptions.infiniteLives == 1) {
        sprintf(lines[4], "Infinite lives:           ON");
    } else {
        sprintf(lines[4], "Infinite lives:          OFF");
    }

    if (hackOptions.infiniteTime > 1) {
        hackOptions.infiniteTime = 0;
    }
    if (hackOptions.infiniteTime < 0) {
        hackOptions.infiniteTime = 1;
    }
    if (hackOptions.infiniteTime == 1) {
        sprintf(lines[5], "Infinite time:            ON");
    } else {
        sprintf(lines[5], "Infinite time:           OFF");
    }

    if (hackOptions.loadFromSavedState > 1) {
        hackOptions.loadFromSavedState = 0;
    }
    if (hackOptions.loadFromSavedState < 0) {
        hackOptions.loadFromSavedState = 1;
    }
    if (hackOptions.loadFromSavedState == 0) {
        sprintf(lines[6], "Begin with saved state:  OFF");
    } else {
        sprintf(lines[6], "Begin with saved state:   ON");
    }

    if (hackOptions.automaticallySaveStatesFreq > 5) {
        hackOptions.automaticallySaveStatesFreq = 0;
    }
    if (hackOptions.automaticallySaveStatesFreq < 0) {
        hackOptions.automaticallySaveStatesFreq = 5;
    }
    if (hackOptions.automaticallySaveStatesFreq == 0) {
        sprintf(lines[7], "Auto-save state:         OFF");
    } else if (hackOptions.automaticallySaveStatesFreq == 1) {
        sprintf(lines[7], "Auto-save state: EVERY 1 min");
    } else if (hackOptions.automaticallySaveStatesFreq == 2) {
        sprintf(lines[7], "Auto-save state: EVERY 5 mins");
    } else if (hackOptions.automaticallySaveStatesFreq == 3) {
        sprintf(lines[7], "Auto-save state: EVERY 10 min");
    } else if (hackOptions.automaticallySaveStatesFreq == 4) {
        sprintf(lines[7], "Auto-save state: EVERY 15 min");
    } else if (hackOptions.automaticallySaveStatesFreq == 5) {
        sprintf(lines[7], "Auto-save state: EVERY 5 secs");
    }

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        char toPrint[0x100];
        if (i == optionsItemIndex) {
            sprintf(toPrint, "> %s", lines[i]);
        } else {
            sprintf(toPrint, "  %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);

        if (blockedLines[i] != 0) {
            layerRenderer_fill(0, 16 + 32, yPos + 3, DEFAULT_WIDTH - 48 - 16, 2, 5);
        }

        yPos += 8;
    }
}