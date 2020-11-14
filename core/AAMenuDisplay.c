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
static int inGameOptionIndex = 0;
static int randomisedGameIndex = 0;
static int persistValuesIndex = 0;

static int majorVersion = 0;
static int minorVersion = 5;

static int DEFAULT_WIDTH = 320;
static int DEFAULT_HEIGHT = 200;

static int queuedMenu = MENU_LISTING_NONE;

static int gameHasStarted = 0;

static HackOptions hackOptions;
static PersistValuesOptions persistValuesOptions;

HackOptions menuDisplay_getHackOptions() {
    return hackOptions;
}

PersistValuesOptions menuDisplay_getPersistValuesOptions() {
    return persistValuesOptions;
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

    FILE *persistValuesReader = fopen("_magicbox/__persistValues.data", "rb");
    if (persistValuesReader) {
        int persistBuffer[0x100];
        fread(persistBuffer, sizeof(int), 0x100, persistValuesReader);
        fclose(persistValuesReader);
        applyPersistValuesFromArray256(persistBuffer);
    } else {
        applyDefaultPersistValues();
    }
}

void applyPersistValuesFromArray256(int array256[]) {
    persistValuesOptions.lives = array256[0];
    persistValuesOptions.rings = array256[1];
    persistValuesOptions.topSpeed = array256[2];
    persistValuesOptions.momentum = array256[3];
    persistValuesOptions.time = array256[4];
}

void applyDefaultPersistValues() {
    persistValuesOptions.lives = 0;
    persistValuesOptions.rings = 0;
    persistValuesOptions.topSpeed = 0;
    persistValuesOptions.momentum = 0;
    persistValuesOptions.time = 0;
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
    hackOptions.shouldWriteToLog = array256[8];
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
    hackOptions.shouldWriteToLog = 0;

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
    options[8] = hackOptions.shouldWriteToLog;

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

    int persistValues[0x100];
    for (int i = 0; i < 0x100; i++) {
        persistValues[i] = 0;
    }
    persistValues[0] = persistValuesOptions.lives;
    persistValues[1] = persistValuesOptions.rings;
    persistValues[2] = persistValuesOptions.topSpeed;
    persistValues[3] = persistValuesOptions.momentum;
    persistValues[4] = persistValuesOptions.time;

    remove("_magicbox/__persistValues.data");
    FILE *persistValuesWriter = fopen("_magicbox/__persistValues.data", "wb");

    for (int i = 0; i < 0x100; i++) {
        fwrite(persistValues, sizeof(int), 0x100, persistValuesWriter);
    }
    fclose(persistValuesWriter);
}

void menuDisplay_showMenu(int menuNum) {
    char tempLog[256];
    sprintf(tempLog, "menuDisplay_showMenu %d", menuNum);
    cartLoader_appendToLog(tempLog);

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

    if (activeMenu == MENU_LISTING_IN_GAME) {
        showInGameOptionsMenu();
    }

    if (activeMenu == MENU_LISTING_RANDOMISED_ROMS) {
        showRandomisedGameMenu();
    }

    if (activeMenu == MENU_LISTING_PERSIST_VALUES) {
        showPersistValuesMenu();
    }
}

void menuDisplay_hideMenu() {
    activeMenu = MENU_LISTING_NONE;

    vdp_setShouldRandomiseColours(0);
    aa_psg_unmute();
    aa_ym2612_unmute();

    layerRenderer_clearLayer(0);
}

void menuDisplay_hideMenuUnlessQueued() {
    if (queuedMenu != MENU_LISTING_NONE) {
        menuDisplay_showMenu(queuedMenu);
        queuedMenu = MENU_LISTING_NONE;
    } else {
        menuDisplay_hideMenu();
    }
}

void refreshMenu() {
    menuDisplay_showMenu(activeMenu);
}

void beginGame() {
    vdp_setShouldRandomiseColours(0);
    aa_psg_unmute();
    aa_ym2612_unmute();
    cartLoader_applyHackOptions(gameHasStarted);
    modConsole_applyHackOptions();
    cartLoader_loadRomAtIndex(chosenGameIndex, 0);

    gameHasStarted = 1;
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
            if (optionsItemIndex == 8) {
                menuDisplay_showMenu(MENU_LISTING_PERSIST_VALUES);
            } else {
                saveHackOptions();
                if (gameHasStarted == 0) {
                    menuDisplay_showMenu(MENU_LISTING_CHOOSE_GAME);
                } else {
                    cartLoader_applyHackOptions(gameHasStarted);
                    modConsole_applyHackOptions();
                    menuDisplay_hideMenu();
                }
            }
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

    if (activeMenu == MENU_LISTING_IN_GAME) {
        if (buttonIndex == INPUT_INDEX_UP) {
            inGameOptionIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            inGameOptionIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C) {
            activateInGameMenuItem();
            menuDisplay_hideMenuUnlessQueued();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_RANDOMISED_ROMS) {
        int romCount = cartLoader_getRomCount();
        if (buttonIndex == INPUT_INDEX_START) {
            // tidy up the menu first!!
            vdp_setShouldRandomiseColours(0);
            aa_psg_unmute();
            aa_ym2612_unmute();

            int romIndex = cartLoder_getLastLoadedIndex();
            if (cartLoader_gameIsBlockedFromRandomiser(romIndex) != 0) {
                cartLoader_loadRandomRom();
            }
            menuDisplay_hideMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_UP) {
            randomisedGameIndex--;
            if (randomisedGameIndex < 0) {
                randomisedGameIndex = romCount - 1;
            }
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            randomisedGameIndex++;
            if (randomisedGameIndex >= romCount) {
                chosenGameIndex = 0;
            }
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_LEFT || buttonIndex == INPUT_INDEX_RIGHT) {
            cartLoader_toggleGameBlockedAtIndex(randomisedGameIndex);
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_PERSIST_VALUES) {
        if (buttonIndex == INPUT_INDEX_UP) {
            persistValuesIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            persistValuesIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_LEFT || buttonIndex == INPUT_INDEX_RIGHT) {
            togglePersistValue(persistValuesIndex);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_START) {
            menuDisplay_showMenu(MENU_LISTING_SETTINGS);
            return 1;
        }
    }

    return 0;
}

void togglePersistValue(int index) {
    if (index == 0) {
        persistValuesOptions.lives = 1 - persistValuesOptions.lives;
    }
    if (index == 1) {
        persistValuesOptions.rings = 1 - persistValuesOptions.rings;
    }
    if (index == 2) {
        persistValuesOptions.topSpeed = 1 - persistValuesOptions.topSpeed;
    }
    if (index == 3) {
        persistValuesOptions.momentum = 1 - persistValuesOptions.momentum;
    }
    if (index == 4) {
        persistValuesOptions.time = 1 - persistValuesOptions.time;
    }
}

void activateInGameMenuItem() {
    // tidy up the menu first!!
    vdp_setShouldRandomiseColours(0);
    aa_psg_unmute();
    aa_ym2612_unmute();

    if (inGameOptionIndex == 1) {
        optionsItemIndex = 0;
        queuedMenu = MENU_LISTING_SETTINGS;
    }
    if (inGameOptionIndex == 2) {
        saveSaveStateForCurrentGame();
        cartLoader_saveAllSaveStatesToDisk();
    }
    if (inGameOptionIndex == 3) {
        cartLoader_loadAllSaveStatesFromDisk();
        cartLoader_loadSaveStateForCurrentGame();
    }
    if (inGameOptionIndex == 4) {
        cartLoader_removeCurrentGameFromRandomiser();
    }
    if (inGameOptionIndex == 5) {
        randomisedGameIndex = cartLoader_getActiveCartIndex();
        queuedMenu = MENU_LISTING_RANDOMISED_ROMS;
    }
    if (inGameOptionIndex == 6) {
        modConsole_queuePanic();
    }
    if (inGameOptionIndex == 7) {
        modConsole_activateReset();
    }

    inGameOptionIndex = 0;
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
    } else if (optionsItemIndex == 8) {
        menuDisplay_showMenu(MENU_LISTING_PERSIST_VALUES);
    } else if (optionsItemIndex == 9) {
        hackOptions.shouldWriteToLog += byAmount;
    }
}

void showTitleMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);

    char titleText[0x100];
    sprintf(titleText, "Alistair's Magic Box V%d.%02d", majorVersion, minorVersion);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, titleText, 5);

    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 32, "HOLD (START + UP + B)", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 40, "for emulator menu", 5);

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

    int lineCount = 10;
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

    if (hackOptions.cooldownOnSwitch > 3) {
        hackOptions.cooldownOnSwitch = 0;
    }
    if (hackOptions.cooldownOnSwitch < 0) {
        hackOptions.cooldownOnSwitch = 1;
    }
    if (hackOptions.cooldownOnSwitch == 0) {
        sprintf(lines[1], "Cooldown after switch:   OFF");
    } else if (hackOptions.cooldownOnSwitch == 1) {
        sprintf(lines[1], "Cooldown after switch: 0.25 sec");
    } else if (hackOptions.cooldownOnSwitch == 2) {
        sprintf(lines[1], "Cooldown after switch: 0.50 sec");
    } else if (hackOptions.cooldownOnSwitch == 3) {
        sprintf(lines[1], "Cooldown after switch: 1.00 sec");
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

    sprintf(lines[8], "Persist values between games >>");

    if (hackOptions.shouldWriteToLog > 1) {
        hackOptions.shouldWriteToLog = 0;
    }
    if (hackOptions.shouldWriteToLog < 0) {
        hackOptions.shouldWriteToLog = 1;
    }
    if (hackOptions.shouldWriteToLog == 0) {
        sprintf(lines[9], "Write to debug log:       OFF");
    } else {
        sprintf(lines[9], "Write to debug log:        ON"); 
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

void showInGameOptionsMenu() {
    cartLoader_appendToLog("showInGameOptionsMenu");

    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "options", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 32, "--- press A/B/C to activate option ---", 5);

    int lineCount = 9;
    char lines[lineCount][0x80];

    if (inGameOptionIndex < 0) {
        inGameOptionIndex = 0;
    }
    if (inGameOptionIndex >= lineCount) {
        inGameOptionIndex = lineCount - 1;
    }

    sprintf(lines[0], "Back to game");
    sprintf(lines[1], "Change hack options");
    sprintf(lines[2], "Save all game states to disk");
    sprintf(lines[3], "Load all game states from disk" );
    sprintf(lines[4], "Remove this game from randomiser" );
    sprintf(lines[5], "Toggle games in randomiser" );
    sprintf(lines[6], "Kill Sonic");
    sprintf(lines[7], "Reset Game");
    sprintf(lines[8], "Back to game");

    int yPos = 48;
    for (int i = 0; i < lineCount; i++) {
        char lineBuf[0x100];
        if (i == inGameOptionIndex) {
            sprintf(lineBuf, ">>   %s", lines[i]);
        } else {
            sprintf(lineBuf, "   %s", lines[i]);

        }
        layerRenderer_writeWord256WithBorder(0, 16, yPos, lineBuf, 5, 1, 0);
        yPos += 8;
    }
}

void showRandomisedGameMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Which games can be randomly", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 24, "switched to?", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to return ---", 5);

    int listHeight = 16;
    int halfListHeight = listHeight / 2;
    int romCount = cartLoader_getRomCount();

    if (randomisedGameIndex < 0) {
        randomisedGameIndex = romCount - 1;
    }
    if (randomisedGameIndex >= romCount) {
        randomisedGameIndex = 0;
    }

    int startIndex = randomisedGameIndex - halfListHeight;
    int endIndex = randomisedGameIndex + halfListHeight;
    if (startIndex < 0) {
        startIndex = 0;
        endIndex = listHeight;
    } 
    if (endIndex >= romCount) {
        endIndex = romCount - 1;
        startIndex = endIndex - listHeight;
        if (startIndex < 0) {
            startIndex = 0;
        }
    }

    char gameRandomStates[romCount][0x100];
    for (int i = 0; i < romCount; i++) {
        char fileNameBuf[0x100];
        cartLoader_getRomFileName(i, fileNameBuf);

        if (cartLoader_gameIsBlockedFromRandomiser(i)) {
            sprintf(gameRandomStates[i], "off: %s", fileNameBuf);
        } else {
            sprintf(gameRandomStates[i], "on:  %s", fileNameBuf);
        }   
    }

    int yPos = 32;
    for (int i = startIndex; i <= endIndex; i++) {
        if (i == randomisedGameIndex) {
            char newNameBuf[0x100];
            for (int j = 0; j < 0xF0; j++) {
                newNameBuf[j + 3] = gameRandomStates[i][j];
            }
            newNameBuf[0] = '>';
            newNameBuf[1] = '>';
            newNameBuf[2] = ' ';
            layerRenderer_writeWord256WithBorder(0, 16, yPos, newNameBuf, 5, 1, 0);
        } else {
            char newNameBuf[0x100];
            for (int j = 0; j < 0xF0; j++) {
                newNameBuf[j + 3] = gameRandomStates[i][j];
            }
            newNameBuf[0] = ' ';
            newNameBuf[1] = ' ';
            newNameBuf[2] = ' ';
            layerRenderer_writeWord256WithBorder(0, 16, yPos, newNameBuf, 5, 1, 0);
        }
        yPos += 8;
    }

}

void showPersistValuesMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "persist values between games", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to confirm ---", 5);

    int lineCount = 5;
    char lines[lineCount][0x80];

    if (persistValuesIndex < 0) {
        persistValuesIndex = lineCount - 1;
    }
    if (persistValuesIndex >= lineCount) {
        persistValuesIndex = 0;
    }

    if (persistValuesOptions.lives == 0) {
        sprintf(lines[0], "Lives:        no");
    } else {
        sprintf(lines[0], "Lives:       yes");
    }
    
    if (persistValuesOptions.rings == 0) {
        sprintf(lines[1], "Rings:        no");
    } else {
        sprintf(lines[1], "Rings:       yes");
    }
    
    if (persistValuesOptions.topSpeed == 0) {
        sprintf(lines[2], "Top speed:    no");
    } else {
        sprintf(lines[2], "Top speed:   yes");
    }

    if (persistValuesOptions.momentum == 0) {
        sprintf(lines[3], "Momentum:     no");
    } else {
        sprintf(lines[3], "Momentum:    yes");
    }

    if (persistValuesOptions.time == 0) {
        sprintf(lines[4], "time:         no");
    } else {
        sprintf(lines[4], "time:        yes");
    }

    


    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        char toPrint[0x100];
        if (i == persistValuesIndex) {
            sprintf(toPrint, "> %s", lines[i]);
        } else {
            sprintf(toPrint, "  %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);
        yPos += 8;
    }
}