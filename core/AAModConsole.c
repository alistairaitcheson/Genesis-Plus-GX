/*

Open Terminal

cd C:\Users\agaitcheson\Documents\Development\Emulation\GenesisPlusGIT\Genesis-Plus-GX
cls
"C:\Program Files (x86)\GnuWin32\bin\make.exe" -f Makefile.libretro

*/

#include "shared.h" // <--- this needs to be included at the top of every file, for compiler reasons I don't understand
#include "AAModConsole.h"
#include "AACommonTypes.h"
#include "genesis.h"
#include "AACartLoader.h"
#include "gamepad.h"
#include "input.h"
#include "AALayerRenderer.h"
#include "AAMenuDisplay.h"

static AAModType activeModType = AAMODTYPE_SWITCH_GAME;

static unsigned int frameCount = 0;

static AAGameListing activeGameListing;

static uint16 lastPadState;
static uint16 padState;

static int hasInitialised = 0;

static int framesUntilClearLayer = 0;

static int switchAfterTimePeriod = 0;
static int switchCooldownPeriod = 0;
static int switchAfterTimeCounter = 0;
static int switchCooldownCounter = 0;

static int shouldSwitchAfterCooldown = 0;

static int saveAllStatesTimePeriod = 0;
static int saveAllStatesTimeCounter = 0;

static int panicCountdown = 0;

void modConsole_initialise() {
    if (hasInitialised == 0) {
        layerRenderer_populateLetters();
        cartLoader_run();
        // showRomList();

        hasInitialised = 1;

        // cartLoader_loadRomAtIndex(0, 0);
        menuDisplay_initialise();
        menuDisplay_showMenu(MENU_LISTING_TITLE);
    }
}

void showRomList() {
    layerRenderer_fill(0, 0, 0, 256, 256, 1);

    layerRenderer_writeWord256(0, 0, 0, "Alistair's Magic Box V0.02", 5);

    layerRenderer_writeWord256(0, 0, 16, "HOLD (START + UP + A + B)", 5);
    layerRenderer_writeWord256(0, 0, 24, "to reset active game", 5);


    layerRenderer_writeWord256(0, 0, 48, "YOUR ROMS:", 5);
    for (int i = 0; i < cartLoader_getRomCount(); i++)
    {
        char fileNameBuf[0x100];
        cartLoader_getRomFileName(i, fileNameBuf);
        layerRenderer_writeWord256(0, 0, 48 + ((i + 1) * 8), fileNameBuf, 5);
    }   

    framesUntilClearLayer = 10 * 60;
}

void modConsole_updateActiveCart() {
    char romHeader[0x20];
    modConsole_getRomHeader(romHeader);
    activeGameListing = cartLoader_getActiveGameListing();

  // cartLoader_appendToLog("modConsole_updateActiveCart - you are playing:");
    // cartLoader_appendToLog(activeGameListing.gameId);
}

void modConsole_applyHackOptions() {
    switchAfterTimeCounter = 0;
    switchAfterTimePeriod = 0;

    switchCooldownCounter = 0;
    switchCooldownPeriod = 0;

    saveAllStatesTimeCounter = 0;

    if (menuDisplay_getHackOptions().switchGameType == 2) {
        switchAfterTimePeriod = 60 * 5;
    }
    if (menuDisplay_getHackOptions().switchGameType == 3) {
        switchAfterTimePeriod = 60 * 10;
    }
    if (menuDisplay_getHackOptions().switchGameType == 4) {
        switchAfterTimePeriod = 60 * 30;
    }
    
    if (menuDisplay_getHackOptions().cooldownOnSwitch == 1) {
        switchCooldownPeriod = 15 * 1;
    }
    if (menuDisplay_getHackOptions().cooldownOnSwitch == 2) {
        switchCooldownPeriod = 30 * 1;
    }
    if (menuDisplay_getHackOptions().cooldownOnSwitch == 3) {
        switchCooldownPeriod = 60 * 1;
    }

    if (menuDisplay_getHackOptions().automaticallySaveStatesFreq == 1) {
        // 1 minute
        saveAllStatesTimePeriod = 60 * 60 * 1;
    }
    if (menuDisplay_getHackOptions().automaticallySaveStatesFreq == 2) {
        // 5 minutes
        saveAllStatesTimePeriod = 60 * 60 * 1;
    }
    if (menuDisplay_getHackOptions().automaticallySaveStatesFreq == 3) {
        // 10 minutes
        saveAllStatesTimePeriod = 60 * 60 * 1;
    }
    if (menuDisplay_getHackOptions().automaticallySaveStatesFreq == 4) {
        // 30 minutes
        saveAllStatesTimePeriod = 60 * 60 * 1;
    }
    if (menuDisplay_getHackOptions().automaticallySaveStatesFreq == 5) {
        // 5 seconds
        saveAllStatesTimePeriod = 60 * 5;
    }
}

void modConsole_updateFrame() {
    lastPadState = padState;
    padState = input.pad[0];

    if (menuDisplay_isShowing() != 0) {
        for (int i = 0; i < 8; i++) {
            if (buttonWasPressedAtIndex(i) != 0) {
                int success = menuDisplay_onButtonPress(i);
                if (success != 0) {
                    break;
                }
            }
        }
        if (frameCount >= 1000) {
            aa_genesis_revertToLastRam();
        }

        // HackOptions hackOpts = menuDisplay_getHackOptions();
        // char optionsDisplay[0x10];
        // sprintf(optionsDisplay, "%d %d %d %d %d %d\n--- %d %d",
        //     hackOpts.switchGameType,
        //     hackOpts.cooldownOnSwitch,
        //     hackOpts.copyVram,
        //     hackOpts.speedUpOnRing,
        //     hackOpts.infiniteLives,
        //     hackOpts.infiniteTime,
        //     switchCooldownPeriod,
        //     switchCooldownCounter
        // );
        // layerRenderer_clearLayer(0);
        // layerRenderer_writeWord256(0, 0, 0, optionsDisplay, 6);

    } else {
        if (framesUntilClearLayer > 0) {
            framesUntilClearLayer--;
            if (framesUntilClearLayer == 0) {
                vdp_clearGraphicLayer(0);
            }
        }

        if (panicCountdown > 0) {
            panicCountdown --;
            modConsole_activatePanic();
        }

        HackOptions hackOpts = menuDisplay_getHackOptions();

        if (hackOpts.automaticallySaveStatesFreq > 0) {
            saveAllStatesTimeCounter ++;
            if (saveAllStatesTimeCounter > saveAllStatesTimePeriod) {
                saveAllStatesTimeCounter = 0;
                saveSaveStateForCurrentGame();
                cartLoader_saveAllSaveStatesToDisk();
            }
        }

        // char optionsDisplay[0x10];
        // sprintf(optionsDisplay, "%d %d %d %d %d %d\n+++ %d %d",
        //     hackOpts.switchGameType,
        //     hackOpts.cooldownOnSwitch,
        //     hackOpts.copyVram,
        //     hackOpts.speedUpOnRing,
        //     hackOpts.infiniteLives,
        //     hackOpts.infiniteTime,
        //     switchCooldownPeriod,
        //     switchCooldownCounter
        // );
        // layerRenderer_clearLayer(0);
        // layerRenderer_writeWord256(0, 0, 0, optionsDisplay, 6);

        if (hackOpts.speedUpOnRing != 0) {
            updateSpeedUpOnRing();
        }
        if (hackOpts.switchGameType == 1) {
            updateSwitchGameOnRing();
        }

        if (hackOpts.switchGameType > 1) {
            switchAfterTimeCounter++;
            if (switchAfterTimeCounter >= switchAfterTimePeriod) {
                switchAfterTimeCounter = 0;
                promptSwitchGame();
            }
        }

        // if (switchCooldownPeriod > 0) {
        //     showCooldownVisualiser();
        // }
        // if (shouldSwitchAfterCooldown) {
        //     switchCooldownCounter++;
        //     if (switchCooldownCounter >= switchCooldownPeriod) {
        //         switchGame();
        //     }       
        // }

        if (hackOpts.infiniteLives != 0) {
            updateLives();
        }
        if (hackOpts.infiniteTime != 0) {
            updateTime();
        }

        // if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
        //     buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
        //     buttonStateAtIndex(INPUT_INDEX_A) != 0) {
        //     modConsole_activatePanic();
        // }
        if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_B) != 0) {
            menuDisplay_showMenu(MENU_LISTING_IN_GAME);
        }
    }

    frameCount++;

    aa_genesis_updateLastRam();
}

void promptSwitchGame() {
    // shouldSwitchAfterCooldown = 1;
    // if (switchCooldownPeriod <= 0) {
        switchGame();
    // }
}

void switchGame() {
    shouldSwitchAfterCooldown = 0;
    switchCooldownCounter = switchCooldownPeriod;
    clearCooldownVisualiser();

    cartLoader_loadRandomRom();
}

void clearCooldownVisualiser() {
    layerRenderer_clearLayer(1);
}

void showCooldownVisualiser() {
    layerRenderer_clearLayer(1);
    if (switchCooldownPeriod > 0) {
        int width = ((vdp_getScreenWidth() - 4) * switchCooldownCounter) / switchCooldownPeriod;
        layerRenderer_fill(1, 0, 0, vdp_getScreenWidth(), 8, 0xFF);
        layerRenderer_fill(1, 2, 2, (vdp_getScreenWidth() - 4), 4, 3);
        layerRenderer_fill(1, 2, 2, width, 4, 4);
    }
}

void updateLives() {
    for (int i = 0; i < 3; i++) {
        if (activeGameListing.livesBytes[i] != 0) {
            aa_genesis_setWorkRam(activeGameListing.livesBytes[i], activeGameListing.livesByteDestinations[i]);
        } else {
            break;
        }
    }
}

void updateTime() {
    for (int i = 0; i < 3; i++) {
        if (activeGameListing.timeBytes[i] != 0) {
            aa_genesis_setWorkRam(activeGameListing.timeBytes[i], activeGameListing.timeByteDestinations[i]);
        } else {
            break;
        }
    }
}

void modConsole_activatePanic() {
    for (int i = 0; i < 3; i++) {
        if (activeGameListing.panicBytes[i] != 0) {
            aa_genesis_setWorkRam(activeGameListing.panicBytes[i], activeGameListing.panicByteDestinations[i]);
        } else {
            break;
        }
    }
}

void modConsole_queuePanic() {
    panicCountdown = 5;
}

void modConsole_activateReset() {
    system_reset();
    aa_genesis_updateLastRam();
}

void updateSpeedUpOnRing() {
    if (ringCountHasChanged() != 0) {
        // speed
        aa_genesis_incrementWorkRamCompoundValueByInt(0xF760, 2, 0x40);
        // acceleration
        aa_genesis_incrementWorkRamCompoundValueByInt(0xF762, 2, 0x08);
    }
}

void updateSwitchGameOnRing() {
    if (switchCooldownCounter > 0) {
        switchCooldownCounter --;
    }

    if (ringCountHasChanged() != 0 && switchCooldownCounter <= 0) {
        // layerRenderer_clearLayer(0);
        // char word[0x20];
        // sprintf(word, "%d", aa_genesis_getWorkRam(activeGameListing.ringByte));

        // layerRenderer_fill(0, 0, 0, 32, 8, 1);
        // layerRenderer_writeWord256(0, 0, 0, word, 5);
        
        promptSwitchGame();
    }
}

int ringCountHasChanged() {
    if (activeGameListing.ringByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(activeGameListing.ringByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(activeGameListing.ringByte);

        if (currentRingCount != 0 && currentRingCount != lastRingCount) {
            return 1;
        }
    }

    if (activeGameListing.specialRingByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(activeGameListing.specialRingByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(activeGameListing.specialRingByte);
        int difference = abs((int)lastRingCount - (int)currentRingCount);

        if (currentRingCount != 0 && currentRingCount != lastRingCount && difference < 6) {
            return 1;
        }
    }

    return 0;
}

void modConsole_getRomHeader(char intoArray[]) {
    if (cart.romsize == 0x400000) {
        writeStringToArray32("SONIC3&KNUCKLES", intoArray);
        return;
    }

    uint8 tempHeader[0x20];
    for (int i = 0; i < 0x20; i++) {
        tempHeader[i] = 0;
    }

    uint8 byteArray[0x20];
    for (int i = 0; i < 0x20; i++) {
        int index = 0x100 + 0x20 + i;
        if (i % 2 == 0) {
            index += 1;
        } else {
            index -= 1;
        }
        uint8 character = getCartValueAtIndex(index);
        tempHeader[i] = character;
    }
    
    uint8 tidiedHeader[0x20];
    int tempIndex = 0;
    for (int i = 0; i < 0x20; i++) {
        if (tempHeader[i] != 0 && tempHeader[i] != ' ') {
            tidiedHeader[tempIndex] = tempHeader[i];
            tempIndex++;
        }
    }
    if (tempIndex < 0x20) {
        tidiedHeader[tempIndex] = '\0';
    }

    for (int i = 0; i < 0x20; i++) {
        if (i <= tempIndex) {
            intoArray[i] = tidiedHeader[i];
        }
        else {
            intoArray[i] = 0;
        }
    }
}

int modconsole_array32sAreEqual(char arrayA[], char arrayB[]) {
    // cartLoader_appendToLog("modconsole_array32sAreEqual");
    // cartLoader_appendToLog(arrayA);
    // cartLoader_appendToLog(arrayB);

    for (int i = 0; i < 0x20; i++) {
        if (arrayA[i] != arrayB[i]) {
            return 0;
        }
        if (arrayA[i] == '\0') {
            break;
        }
    }
    return 1;
}

int getButtonState(uint16 whichInput) {
    // char padStateAsString[0x20];
    // sprintf(padStateAsString, "%d", padState & whichInput);
    // cartLoader_appendToLog(padStateAsString);

    if (padState & whichInput != 0) {
        return 1;
    }
    return 0;
}

int getLastButtonState(uint16 whichInput) {
    if (lastPadState & whichInput != 0) {
        return 1;
    }
    return 0;
}

int buttonWasReleased(uint16 whichInput) {
    if (getLastButtonState(whichInput) != 0 && getButtonState(whichInput) == 0) {
        return 1;
    }
    return 0;
}

int buttonWasPressed(uint16 whichInput) {
    if (getLastButtonState(whichInput) == 0 && getButtonState(whichInput) != 0) {
        return 1;
    }
    return 0;
}

int buttonWasReleasedAtIndex(int index) {
    if (lastButtonStateAtIndex(index) != 0 && buttonStateAtIndex(index) == 0) {
        return 1;
    }
    return 0;
}

int buttonWasPressedAtIndex(int index) {
    if (lastButtonStateAtIndex(index) == 0 && buttonStateAtIndex(index) != 0) {
        return 1;
    }
    return 0;
}

int lastButtonStateAtIndex(int index) {
    uint testNum = 1;
    for (int i = 0; i < index; i++) {
        testNum = testNum * 2;
    }

    int result = ((int)((lastPadState % 0x100) & testNum) % 0x100);

    // for (int i = 0; i < 0x10; i++) {
    //     for (int j = 0; j < testNum; j++) {
    //         vdp_setGraphicLayerPixel(0, j, (index + 3) * 10, 9);
    //     }
    //     vdp_setGraphicLayerPixel(0, result, ((index + 3) * 10) + 1, 5);
    // }

    if (result == 0) { // why does this always return false?
        return 0;
    } else {
        return 1;
    }
}

int buttonStateAtIndex(int index) {
    uint testNum = 1;
    for (int i = 0; i < index; i++) {
        testNum = testNum * 2;
    }

    int result = ((int)((padState % 0x100) & testNum) % 0x100);

    // for (int i = 0; i < 0x10; i++) {
    //     for (int j = 0; j < testNum; j++) {
    //         vdp_setGraphicLayerPixel(0, j, (index + 3) * 10, 9);
    //     }
    //     vdp_setGraphicLayerPixel(0, result, ((index + 3) * 10) + 1, 5);
    // }

    if (result == 0) {
        return 0;
    } else {
        return 1;
    }
}