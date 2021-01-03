/*

Open Terminal

cd C:\Users\agait\OneDrive\Documents\Development\GenesisPlusGX\Genesis-Plus-GX
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

static int shouldApplyCacheNextFrame = 0;

static int valueWriteTimeCounter = 0;

static int countdownToSummonMenu = 0;

static int countdownUntilRingSwitch = 0;

static int countdownUntilLogRamState = 0;

static int countdownUntilUnpause = 0;

void modConsole_flagToLogRamState() {
    countdownUntilLogRamState = 1;
}

void modConsole_flagToApplyCache() {
    shouldApplyCacheNextFrame = 1;
}

void modConsole_flagToSummonMenu() {
    countdownToSummonMenu = 60;
}

void modConsole_setCountdownUntilRingSwitch(int toValue) {
    countdownUntilRingSwitch = toValue;
}

void modConsole_initialise() {
    if (hasInitialised == 0) {
        layerRenderer_populateLetters();
        menuDisplay_initialise();
        cartLoader_run();
        // showRomList();

        hasInitialised = 1;

        // cartLoader_loadRomAtIndex(0, 0);
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
    cartLoader_appendToLog("modConsole_updateActiveCart - active ROM is:");
    cartLoader_appendToLog(romHeader);

    activeGameListing = cartLoader_getActiveGameListing();

    cartLoader_appendToLog("modConsole_updateActiveCart - you are playing:");
    cartLoader_appendToLog(activeGameListing.gameId);
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

    if (menuDisplay_getHackOptions().shouldSortColours != 0) {
        vdp_setShouldSortPixels(1);
    } else {
        vdp_setShouldSortPixels(0);
    }

    if (menuDisplay_getHackOptions().shouldHideLayers == 0) {
        vdp_setShouldHideSprites(0);
        vdp_setShouldHideBackgrounds(0);
    } else if (menuDisplay_getHackOptions().shouldHideLayers == 1) {
        vdp_setShouldHideSprites(1);
        vdp_setShouldHideBackgrounds(0);
    } else if (menuDisplay_getHackOptions().shouldHideLayers == 2) {
        vdp_setShouldHideSprites(0);
        vdp_setShouldHideBackgrounds(1);
    }

    if (menuDisplay_getHackOptions().limitedColourType == 0) {
        vdp_setShouldLimitColourPalettes(0);
    } else if (menuDisplay_getHackOptions().limitedColourType == 1) {
        vdp_setShouldLimitColourPalettes(1);
        vdp_generateAlistairSortedColours(2);
    } else if (menuDisplay_getHackOptions().limitedColourType == 2) {
        vdp_setShouldLimitColourPalettes(1);
        vdp_generateAlistairSortedColours(3);
    } else if (menuDisplay_getHackOptions().limitedColourType == 3) {
        vdp_setShouldLimitColourPalettes(1);
        vdp_generateAlistairSortedColours(4);
    } else if (menuDisplay_getHackOptions().limitedColourType == 4) {
        vdp_setShouldLimitColourPalettes(1);
        vdp_generateAlistairSortedColours(5);
    } else if (menuDisplay_getHackOptions().limitedColourType == 5) {
        vdp_setShouldLimitColourPalettes(1);
        vdp_generateAlistairSortedColours(10);
    }
}

void modConsole_updateFrame() {
    lastPadState = padState;
    padState = input.pad[0];

    if (menuDisplay_isShowing() != 0) {
        // vdp_clearGraphicLayer(1);
        // for (int i = 0; i < 8; i++) {
        //     if (buttonWasPressedAtIndex(i)) {
        //         layerRenderer_fill(1, 8 * i, 0, 8, 8, 0xff);
        //     }
        // }

        int translatedButtons[8];
        for (int i = 0; i < 8; i++) {
            translatedButtons[i] = i;
        }
        // if (vdp_isMasterSystem() != 0) {
        //     translatedButtons[INPUT_INDEX_A] = INPUT_INDEX_C;
        //     translatedButtons[INPUT_INDEX_B] = INPUT_INDEX_START;
        // }

        for (int i = 0; i < 8; i++) {
            int buttonId = translatedButtons[i];
            if (buttonWasPressedAtIndex(buttonId) != 0) {
                int success = menuDisplay_onButtonPress(buttonId);
                if (success != 0) {
                    break;
                }
            }
        }
        // if (frameCount >= 1000) {
        //     aa_genesis_revertToLastRam();
        // }

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
        if (shouldApplyCacheNextFrame > 0) {
            shouldApplyCacheNextFrame--;
            cartLoader_restoreCarriedOverData();
            aa_genesis_updateLastRam();
        }

        if (framesUntilClearLayer > 0) {
            framesUntilClearLayer--;
            if (framesUntilClearLayer == 0) {
                vdp_clearGraphicLayer(0);
            }
        }

        menuDisplay_updateRamDetective();
        menuDisplay_renderRamDetective();
        menuDisplay_renderPixelDetective();

        if (countdownUntilLogRamState > 0) {
            countdownUntilLogRamState --;
            if (countdownUntilLogRamState == 0) {
                menuDisplay_logRamStateToTrackedValues();
            }
        }

        if (panicCountdown > 0) {
            panicCountdown --;
            // char tempLog[0x100];
            // sprintf(tempLog, "Panic countdown %d", panicCountdown);
            // cartLoader_appendToLog(tempLog);
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

        if (hackOpts.overwriteLevelType > 0) {
            overwriteLevelOnRing();
        }

        if (hackOpts.switchGameType > 1) {
            switchAfterTimeCounter++;
            if (switchAfterTimeCounter >= switchAfterTimePeriod) {
                switchAfterTimeCounter = 0;
                promptSwitchGame();
            }
        }

        if (countdownUntilRingSwitch > 0) {
            countdownUntilRingSwitch--;
            if (countdownUntilRingSwitch == 0) {
                promptSwitchGame();
            }
        }

        if (countdownUntilUnpause > 0) {
            countdownUntilUnpause --;
            if (countdownUntilUnpause == 0) {
                unpauseGame();
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

        valueWriteTimeCounter++;
        if (valueWriteTimeCounter > activeGameListing.valueWriteDuration) {
            if (hackOpts.infiniteLives != 0) {
                updateLives();
            }
            if (hackOpts.infiniteTime != 0) {
                updateTime();
            }
        }

        if (hackOpts.shouldShowSwapCount != 0) {
            char counterText[0x40];
            sprintf(counterText, "SWAPS: %08d", cartLoader_getSwapCount());
            int lengthOfText = lengthOfString256(counterText);

            vdp_clearGraphicLayer(2);
            layerRenderer_fill(2, 0, vdp_getScreenHeight() - 8, 8 * lengthOfText, 8, 0xFF);
            layerRenderer_writeWord256(2, 0, vdp_getScreenHeight() - 8, counterText, 0x5);
        } else {
            vdp_clearGraphicLayer(2);
        }


        // if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
        //     buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
        //     buttonStateAtIndex(INPUT_INDEX_A) != 0) {
        //     modConsole_activatePanic();
        // }
        if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_B) != 0) {
            cartLoader_cacheSaveStateBeforeMenu();
            menuDisplay_showMenu(MENU_LISTING_IN_GAME);
        } else if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_C) != 0) {
            cartLoader_cacheSaveStateBeforeMenu();
            menuDisplay_showMenu(MENU_LISTING_IN_GAME);
        } else if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_A) != 0) {
            cartLoader_cacheSaveStateBeforeMenu();
            menuDisplay_showMenu(MENU_LISTING_IN_GAME);
        } else if (
            // instal-kill!!
            buttonStateAtIndex(INPUT_INDEX_DOWN) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_B) != 0)
        {
            modConsole_activatePanic();
        }

        // // show what buttons are being pressed!
        // char controlsTextBuf[0x40];
        // sprintf(controlsTextBuf, "........");
        // if (buttonStateAtIndex(INPUT_INDEX_UP) != 0) {
        //     controlsTextBuf[0] = 'U';
        // }
        // if (buttonStateAtIndex(INPUT_INDEX_DOWN) != 0) {
        //     controlsTextBuf[1] = 'D';
        // }
        // if (buttonStateAtIndex(INPUT_INDEX_LEFT) != 0) {
        //     controlsTextBuf[2] = 'L';
        // }
        // if (buttonStateAtIndex(INPUT_INDEX_RIGHT) != 0) {
        //     controlsTextBuf[3] = 'R';
        // }
        // if (buttonStateAtIndex(INPUT_INDEX_A) != 0) {
        //     controlsTextBuf[4] = 'A';
        // }
        // if (buttonStateAtIndex(INPUT_INDEX_B) != 0) {
        //     controlsTextBuf[5] = 'B';
        // }
        // if (buttonStateAtIndex(INPUT_INDEX_C) != 0) {
        //     controlsTextBuf[6] = 'C';
        // }
        // if (buttonStateAtIndex(INPUT_INDEX_START) != 0) {
        //     controlsTextBuf[7] = 'S';
        // }
        // vdp_clearGraphicLayer(2);
        // layerRenderer_fill(2, 0, 0, 8 * 8, 8, 0xFF);
        // layerRenderer_writeWord256(2, 0, 0, controlsTextBuf, 0x5);


        if (countdownToSummonMenu > 0) {
            countdownToSummonMenu--;
            if (countdownToSummonMenu == 0) {
                menuDisplay_showMenu(MENU_LISTING_IN_GAME);
            }
        }
    }

    frameCount++;

    aa_genesis_updateLastRam();
}

void unpauseGame() {
    if (activeGameListing.unpauseByte > 0) {
        // this doesn't work yet
        // cartLoader_appendToLog("unpausing");
        // aa_genesis_setWorkRam(activeGameListing.unpauseByte, activeGameListing.unpauseByteDestination);
    } else {
        // cartLoader_appendToLog("did not unpause");
    }
}

void overwriteLevelOnRing() {
    if (ringCountHasChanged() != 0) {
        AALevelEditListing levelEdits = cartLoader_getActiveLevelEditListing();
        HackOptions hackOpts = menuDisplay_getHackOptions();
        if (levelEdits.endByte > 0 && levelEdits.endByte > levelEdits.startByte) {
            int cycleCount = 10;
            if (hackOpts.overwriteLevelDifficulty == 1) {
                cycleCount = 20;
            }
            if (hackOpts.overwriteLevelDifficulty == 2) {
                cycleCount = 50;
            }

            for (int i = 0; i < cycleCount; i++) {
                unsigned int value = 0;
                if (hackOpts.overwriteLevelType == 1) {
                    value = rand() % 0x100;
                }
                unsigned int index = (rand() % (levelEdits.endByte - levelEdits.startByte)) + levelEdits.startByte;
                aa_genesis_setWorkRam(index, value);
            }
        }
    }
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
    cartLoader_appendToLog("modConsole_activatePanic");
    for (int i = 0; i < 3; i++) {

        if (activeGameListing.panicBytes[i] != 0) {
            aa_genesis_setWorkRam(activeGameListing.panicBytes[i], activeGameListing.panicByteDestinations[i]);
            char tempLog[0x100];
            sprintf(tempLog, "Setting panic byte at index %i (%04X -> %02X", i, activeGameListing.panicBytes[i], activeGameListing.panicByteDestinations[i]);
            cartLoader_appendToLog(tempLog);
        } else {
            char tempLog[0x100];
            sprintf(tempLog, "Nothing at index %i", i);
            cartLoader_appendToLog(tempLog);
            break;
        }
    }

    countdownUntilUnpause = 5;
}

void modConsole_queuePanic() {
    panicCountdown = 60;
    cartLoader_appendToLog("modConsole_queuePanic");
}

void modConsole_activateReset() {
    system_reset();
    aa_genesis_updateLastRam();
}

void updateSpeedUpOnRing() {
    if (ringCountHasChanged() != 0) {
        if (cartLoader_getActiveGameListing().accelerationType == 1) {
            cartLoader_appendToLog("Increasing Sonic 2D speed");

            // speed
            aa_genesis_incrementWorkRamCompoundValueByInt(0xF760, 2, 0x40);
            // acceleration
            aa_genesis_incrementWorkRamCompoundValueByInt(0xF762, 2, 0x08);
        }
        if (cartLoader_getActiveGameListing().accelerationType == 2) {
            // I don't think this does anything because I don't think Sonic's running
            // speed is held in RAM

            /*
            cartLoader_appendToLog("Increasing Sonic3DBlast speed");
            char logText[0x100];
            sprintf(logText, "X: From %02X, %02X", aa_genesis_getWorkRam(0xC204), aa_genesis_getWorkRam(0xC205));
            cartLoader_appendToLog(logText);
            // x speed
            for (int i = 0xC1F4; i < 0xC1F8; i++) {
                aa_genesis_incrementWorkRamCompoundValueByInt(i, 1, 0x40);
            }
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC204, 1, 0x40);
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC205, 1, 0x40);
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC206, 1, 0x40);
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC207, 1, 0x40);
            sprintf(logText, "X: To   %02X, %02X", aa_genesis_getWorkRam(0xC204), aa_genesis_getWorkRam(0xC205));
            cartLoader_appendToLog(logText);
            // y speed
            sprintf(logText, "Y: From %02X, %02X", aa_genesis_getWorkRam(0xC206), aa_genesis_getWorkRam(0xC207));
            cartLoader_appendToLog(logText);
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC208, 1, 0x40);
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC209, 1, 0x40);
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC20A, 1, 0x40);
            aa_genesis_incrementWorkRamCompoundValueByInt(0xC20B, 1, 0x40);
            sprintf(logText, "Y: To   %02X, %02X", aa_genesis_getWorkRam(0xC206), aa_genesis_getWorkRam(0xC207));
            cartLoader_appendToLog(logText);
            */
        }
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
        if (activeGameListing.ringSwitchCooldown > 0) {
            countdownUntilRingSwitch = activeGameListing.ringSwitchCooldown;
        } else {
            promptSwitchGame();
        }
    }

    if (switchAfterTimeCounter <= 0) {
        cartLoader_checkPixelTrackerForStateChange();
    }
}

int ringCountHasChanged() {
    if (activeGameListing.ringByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(activeGameListing.ringByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(activeGameListing.ringByte);

        if (currentRingCount != 0 && currentRingCount != lastRingCount) {
            char word[0x100];
            sprintf(word, "Ring count went from %02X to %02X, frame %d", lastRingCount, currentRingCount, frameCount);
            cartLoader_appendToLog(word);
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

    for (int i = 0; i < 8; i++) {
        if (activeGameListing.bytesToTestForChange[i] != 0) {
            unsigned int lastVal = aa_genesis_getLastWorkRam(activeGameListing.bytesToTestForChange[i]);
            unsigned int currentVal = aa_genesis_getWorkRam(activeGameListing.bytesToTestForChange[i]);

            if (lastVal != currentVal) {
                return 1;
            }
        }
    }

    AAScoreMonitorListing scoreListing = cartLoader_getActiveScoreMonitorListing();
    int lastScore = 0;
    int currentScore = 0;
    int multiplier = 1;

    // P1 score
    for (int i = 0; i < 8; i++) {
        if (scoreListing.scoreBytes[i] > 0 && scoreListing.scoreBytes[i] < 0x10000) {
            unsigned int lastScoreVal = aa_genesis_getLastWorkRam(scoreListing.scoreBytes[i]);
            unsigned int currentScoreVal = aa_genesis_getWorkRam(scoreListing.scoreBytes[i]);

            //change the below for different calculation types
            if (scoreListing.calculatationType == 0) {
                lastScore += lastScoreVal * multiplier;
                currentScore += currentScoreVal * multiplier;
                multiplier *= 0x100;
            } else if (scoreListing.calculatationType == 1) {
                unsigned int lastScoreLowDigit = lastScoreVal % 0x10;
                unsigned int lastScoreHighDigit = lastScoreVal / 0x10;
                unsigned int currentScoreLowDigit = currentScoreVal % 0x10;
                unsigned int currentScoreHighDigit = currentScoreVal / 0x10;

                lastScore += (lastScoreLowDigit + (10 * lastScoreHighDigit)) * multiplier;
                currentScore += (currentScoreLowDigit + (10 * currentScoreHighDigit)) * multiplier;
                multiplier *= 100;
            }
        } else {
            break;
        }
    }

    int blockedBecauseZero = 0;
    if (scoreListing.blockJumpFromZero != 0) {
        if (lastScore == 0) {
            blockedBecauseZero = 1;
        }
    }
    if (multiplier > 1 && currentScore > lastScore + scoreListing.scoreJumpForTrigger && blockedBecauseZero == 0) {
        return 1;
    }

    // P2 score
    lastScore = 0;
    currentScore = 0;
    multiplier = 1;
    for (int i = 0; i < 8; i++) {
        if (scoreListing.scoreBytesP2[i] > 0 && scoreListing.scoreBytesP2[i] < 0x10000) {
            unsigned int lastScoreVal = aa_genesis_getLastWorkRam(scoreListing.scoreBytesP2[i]);
            unsigned int currentScoreVal = aa_genesis_getWorkRam(scoreListing.scoreBytesP2[i]);

            //change the below for different calculation types
            lastScore += lastScoreVal * multiplier;
            currentScore += currentScoreVal * multiplier;
            multiplier *= 0x100;
        } else {
            break;
        }
    }

    blockedBecauseZero = 0;
    if (scoreListing.blockJumpFromZero != 0) {
        if (lastScore == 0) {
            blockedBecauseZero = 1;
        }
    }
    if (multiplier > 1 && currentScore > lastScore + scoreListing.scoreJumpForTrigger && blockedBecauseZero == 0) {
        return 1;
    }

    return 0;
}

void modConsole_getMasterSystemProductId(char intoArray[]) {
    sprintf(intoArray, "%02X%02X%01X", getCartValueAtIndex(0x7FFC), getCartValueAtIndex(0x7FFD), getCartValueAtIndex(0x7FFE) / 0x10);
}

void modConsole_getRomFingerprint(char intoArray[], int location) {
    for (int i = 0; i < 0x20; i++) {
        intoArray[i] = getCartValueAtIndex(location + i);
    }
}

void modConsole_getRomHeader(char intoArray[]) {
    // cartLoader_appendToLog("***** modConsole_getRomHeader");

    // if (frameCount > 100) {
        // for (int i = 0; i < 0x200; i++) {
        //     uint8 character = getCartValueAtIndex(i);
        //     char logMsg[0x20];
        //     sprintf(logMsg, "%02X: %02X", i, character);
        //     cartLoader_appendToLog(logMsg);
        // }
    // }

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
    // cartLoader_appendToLog(tempHeader);
    
    uint8 tidiedHeader[0x20];
    int tempIndex = 0;
    for (int i = 0; i < 0x20; i++) {
        // char logMsg[0x20];
        // sprintf(logMsg, "%02X: %02X", i, tempHeader[i]);
        // cartLoader_appendToLog(logMsg);
        if (tempHeader[i] != 0 && tempHeader[i] != ' ') {
            tidiedHeader[tempIndex] = tempHeader[i];
            tempIndex++;
        }
    }
    if (tempIndex < 0x20) {
        tidiedHeader[tempIndex] = '\0';
    }

    // if no luck getting a header, just check a weird location
    if (tempIndex == 0) {
        modConsole_getRomFingerprint(tidiedHeader, 0x10000);
        tempIndex = 0x1C;
    }

    for (int i = 0; i < 0x20; i++) {
        if (i <= tempIndex) {
            intoArray[i] = tidiedHeader[i];
        }
        else {
            intoArray[i] = 0;
        }
    }

    if (cart.romsize == 0x400000 && modconsole_array32sAreEqual("SONIC&KNUCKLES", intoArray) != 0) {
        writeStringToArray32("SONIC3&KNUCKLES", intoArray);
        return;
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

int lengthOfString256(char string256[]) {
    for (int i = 0; i < 0x100; i++) {
        if (string256[i] == 0 || string256[i] == '\0') {
            return i;
        }
    }
    return 0x100;
}