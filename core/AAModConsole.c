/*

Open Terminal

cd C:\Users\agait\Documents\Development\GenesisPlusGX\Genesis-Plus-GX
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
#include "vdp_render.h"

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

static char queuedNetworkMessage[0x100];
static int networkMessageLength = 0;

static int snapEffectTime;
static int snapEffectMaxTime = 6;
static int snapEffectHeight[0x10];
static int snapEffectWidth[0x10];
static int snapEffectOffset[0x10];

static int removeColourTimer = 0;
static int healColourTimer = 0;

static int postRingEffectCooldownTimePerGame[MAX_ROMS];

static int rewindFrameCounter = 0;
static int framesBetweenRewindCache = 240;
static int framesHeldDownRewindButtons = 0;

static int rewindSymbolColour = 0x10;

static int playerDeathCount = 0;

static uint8 holdValues[0x10000]; 
static int holdDurations[0x10000]; 

static int holdEffectFramesLeft = 0;
static int holdEffectDuration = 300;

static int didModifyLives = 0;

static int hasDismissedStartupHint = 0;

static int vramWriteOffset = 0;

static int shouldRewind = 0;

static int pendingRingTriggers = 0;
static int intervalBetweenPendingTriggers = 15;
static int pendingRingTriggerTimer = 0;
static int hasFlaggedPendingRingsThisFrame = 0;
static int bossRushElapsedFrames = 0;
static int countdownToApplyBossRushRings = 0;

static int shouldShuffleController = 0;
static int shuffleControllerCountdown = 0;
static int SHUFFLE_CONTROLLER_DURATION = 30 * 60; //30 * 60;
static int showShuffleAlertCountdown = 0;
static int SHOW_SHUFFLE_ALERT_DURATION = 2 * 60;

static int shouldCheckForIdleMode = 0;
static int idleModeFrameCount = 0;
static int idleModeActive = 0;
//3 minutes
static int MAX_FRAMES_FOR_IDLE_MODE = 60 * 60 * 3;

int countdownToUnrandomiseColours = 0;

static int headingTextScrollPixels = 0;
static int headingTextScrollSubpixels = 0;
static int headingColour = 0x5;
static int HEADING_TEXT_SUBPIXEL_PER_PIXEL = 5;

static char lastTerminalEffect[100];
static int terminalEffectDisplayCountdown = 0;
static int TERMINAL_EFFECT_DISPLAY_DURATION = 60 * 3;
static int terminalRotorValues[8];

static int hasLEDdisplay = 0;
static int idleModeCooldown = 0;

static int debug_lastMusicTempo = 0;
static int debug_haltMusicCountdown = 0;

static int ninesChallengeElapsedFrames = 0;

static int diedThisFrame = 0;

static int flashRingsToGoCountTime = 0;
static int flashRingsToGoPeriod = 5;
static int flashRingsToGoDuration = 180;

static int consecutiveEventCount = 0;
static int ninesOpponentLeadCountDown = 0;
static int ninesOpponentLeadCoundDownDuration = 60 * 5;

void requestFlashRingsToGo() {
    flashRingsToGoCountTime = flashRingsToGoDuration;
}

void checkToHaltMusic() {
    if (shouldUseBossRush() && menuDisplay_getBossRushOptions().shouldUseExternalMusic /*&& aa_genesis_getWorkRam(0xF601) < 0x80*/) {
        // vdp_clearGraphicLayer(3);
        // layerRenderer_writeWord256(3, 0, 0, "Boss rush music silencer on", 0x5);

        int shouldOverrideNow = 0;
        AAMusicOverrideListing musicListing = cartLoader_getActiveMusicOverrideListing();
        if (musicListing.byteToCheckForTrackChange != 0) {
            int currentValue = 0;
            if (musicListing.shouldEditZ80 == 0) {
                currentValue = aa_genesis_getWorkRam(musicListing.byteToCheckForTrackChange);
            } else {
                // check z80 ram
                currentValue = aa_genesis_getZ80Ram(musicListing.byteToCheckForTrackChange);
                // if (musicListing.byteStringCheckForTrackChange > 0) {
                //     currentValue = 0;
                //     for (int i = 0; i < musicListing.byteStringCheckForTrackChange; i++) {
                //         currentValue += aa_genesis_getZ80Ram(musicListing.byteToCheckForTrackChange + i);
                //     }
                // }
            }

            // check work ram
            if (musicListing.lastTrackChangeValue != currentValue && frameCount % 5 == 0) {
                musicListing.lastTrackChangeValue = currentValue;
                cartLoader_beginCurrentHaltCountdown();
                shouldOverrideNow = 1;

                if (musicListing.shouldEditZ80 == 0) {
                    aa_genesis_setWorkRam(musicListing.byteToCheckForTrackChange, musicListing.valueToWriteIntoTrackChangedSlot);
                } else {
                    // check z80 ram
                    aa_genesis_setZ80Ram(musicListing.byteToCheckForTrackChange, musicListing.valueToWriteIntoTrackChangedSlot);
                    // if (musicListing.byteStringCheckForTrackChange > 0) {
                    //     for (int i = 0; i < musicListing.byteStringCheckForTrackChange; i++) {
                    //         aa_genesis_setZ80Ram(musicListing.byteToCheckForTrackChange + i, 0);
                    //     }
                    // }
                }

            }

            // char detailsBuf[0x100];
            // sprintf(detailsBuf, "(%i) %04X - %02X > %02X - ctdn %i - ovr %i", musicListing.shouldEditZ80, musicListing.byteToCheckForTrackChange, musicListing.lastTrackChangeValue, currentValue, musicListing.haltMusicCountdown, shouldOverrideNow);
            // layerRenderer_writeWord256(3, 0, 8, detailsBuf, 0x5);

            // char detailsBuf2[0x100];
            // sprintf(detailsBuf2, "(%04X = %02X / %02X) (%04X = %02X / %02X)", 
            //     musicListing.byteToWriteToForNoMusic, aa_genesis_getZ80Ram(musicListing.byteToWriteToForNoMusic), musicListing.valueToWriteForNoMusic, 
            //     musicListing.secondByteToWriteToForNoMusic, aa_genesis_getZ80Ram(musicListing.secondByteToWriteToForNoMusic), musicListing.secondValueToWriteForNoMusic);
            // layerRenderer_writeWord256(3, 0, 16, detailsBuf2, 0x5);

            if (musicListing.haltMusicCountdown > 0 || shouldOverrideNow != 0) {
                // if (frameCount % 5 == 0) {
                enforceHaltMusic();
                // }
                
                cartLoader_reduceCurrentHaltCountdown();

                // char detailsBuf3[0x100];
                // sprintf(detailsBuf3, "(%04X = %02X / %02X) (%04X = %02X / %02X)", 
                //     musicListing.byteToWriteToForNoMusic, aa_genesis_getZ80Ram(musicListing.byteToWriteToForNoMusic), musicListing.valueToWriteForNoMusic, 
                //     musicListing.secondByteToWriteToForNoMusic, aa_genesis_getZ80Ram(musicListing.secondByteToWriteToForNoMusic), musicListing.secondValueToWriteForNoMusic);
                // layerRenderer_writeWord256(3, 0, 24, detailsBuf3, 0x5);
            }
        }
        
        // // nuclear option in case of Sonic 3
        if (cartLoader_getActiveCartIndex() == 3) {
            // for (int i = 0x1078; i < 0x114A; i++) {
            for (int i = 0x10E0; i < 0x1100; i++) {//7AB3231B
                aa_genesis_setZ80Ram(i, 0);
            }
        }

        // // nuclear option in case of Sonic & knuckles
        if (cartLoader_getActiveCartIndex() == 4) {
            // char skBufTop[0x100];
            // sprintf(skBufTop, "");
            // char skBufBottom[0x100];
            // sprintf(skBufBottom, "");

            for (int i = 0x1FE0; i < 0x1FF0; i++) {
                // sprintf(skBufTop, "%s %02X", skBufTop, aa_genesis_getZ80Ram(i));
                aa_genesis_setZ80Ram(i, 0);
            }
            for (int i = 0x1FF0; i < 0x2000; i++) {
                // sprintf(skBufBottom, "%s %02X", skBufBottom, aa_genesis_getZ80Ram(i));
                aa_genesis_setZ80Ram(i, 0);
            }
            // layerRenderer_writeWord256(3, 0, 8, skBufTop, 0x5);
            // layerRenderer_writeWord256(3, 0, 16, skBufBottom, 0x5);
        }

        // // nuclear option in case of Sonic 3D Blast
        if (cartLoader_getActiveCartIndex() == 6) {
            aa_genesis_setZ80Ram(0x1FF8, 0);
            aa_genesis_setZ80Ram(0x1FF9, 0);
        }
    }
}

void enforceHaltMusic() {
    AAMusicOverrideListing musicListing = cartLoader_getActiveMusicOverrideListing();


    if (musicListing.shouldEditZ80 == 0) {
        // set the "stop all sounds" flag
        aa_genesis_setWorkRam(musicListing.byteToWriteToForNoMusic, musicListing.valueToWriteForNoMusic);
        // set the tempo to 0
        if (musicListing.haltMusicCountdown < 3) {
            aa_genesis_setWorkRam(musicListing.byteToCheckForTrackChange, musicListing.valueToWriteIntoTrackChangedSlot);
        }

        if (musicListing.secondByteToWriteToForNoMusic != 0) {
            aa_genesis_setWorkRam(musicListing.secondByteToWriteToForNoMusic, musicListing.secondValueToWriteForNoMusic);
        }
    } else {
        // set the "stop all sounds" flag
        aa_genesis_setZ80Ram(musicListing.byteToWriteToForNoMusic, musicListing.valueToWriteForNoMusic);
        // if (musicListing.secondByteToWriteToForNoMusic != 0) {
        //     aa_genesis_setZ80Ram(musicListing.secondByteToWriteToForNoMusic, musicListing.secondValueToWriteForNoMusic);
        // }

        // if (musicListing.byteStringLengthToWriteForNoMusic > 0) {
        //     for (int i = 0; i < musicListing.byteStringLengthToWriteForNoMusic; i++) {
        //         aa_genesis_setZ80Ram(musicListing.byteToWriteToForNoMusic + i, 0);
        //     }
        // }

        // // tricky option in case of Sonic 3
        // if (cartLoader_getActiveCartIndex() == 3 && frameCount % 5 == 0) {
        //     queueZ80MemoryChange(0x1FF2, 0);
        //     queueZ80MemoryChange(0x1FF3, 0);

        //     queueZ80MemoryChange(0x1FF4, 0);
        //     queueZ80MemoryChange(0x1FF5, 0);
        // }

        // set the tempo to 0
        aa_genesis_setZ80Ram(musicListing.byteToCheckForTrackChange, musicListing.valueToWriteIntoTrackChangedSlot);

        // if (musicListing.byteStringCheckForTrackChange > 0) {
        //     for (int i = 0; i < musicListing.byteStringCheckForTrackChange; i++) {
        //         aa_genesis_setZ80Ram(musicListing.byteToCheckForTrackChange + i, 0);
        //     }
        // }
    }
    // when we load new music its tempo will not be 0
    // so we can check "oh the music has changed!"
    // and fire this flag
}


void setHasLEDDisplay(int toValue) {
    hasLEDdisplay = toValue;
}

void reportToLED(char actionId[]) {
    // char tempMessage[10];
    // sprintf(tempMessage, "?%s", actionId); 
    // cartLoader_writeActionToNetwork(tempMessage);
}

void incrementTerminalRotorValue(int whichRotor, int amount) {
    terminalRotorValues[whichRotor % 8] += amount;
}

void setShouldCheckForIdleMode(int toValue) {
    shouldCheckForIdleMode = toValue;
    idleModeFrameCount = 0;

    headingTextScrollPixels = -(vdp_getScreenWidth() + 50);
    headingColour = (rand() % 10) + 0x5;
}

void beginIdleMode() {
    setShouldCheckForIdleMode(0);
    idleModeFrameCount = 0;
    

    cartLoader_setAllGamesAsBlocked();
    cartLoader_setRandomSelectionOfGamesAsUnblocked(4);

    // cartLoader_unblockGamesWithCartNumber(1);
    // cartLoader_unblockGamesWithCartNumber(2);
    // cartLoader_unblockGamesWithCartNumber(3);
    // cartLoader_unblockGamesWithCartNumber(4);

    cartLoader_clearSaveStates();
    menuDisplay_beginIdleMode();
    promptSwitchGame();

    modConsole_applyHackOptions();

    idleModeActive = 1;

    setShouldShuffleController(0);
}

void endIdleMode() {
    idleModeActive = 0;
    setShouldShuffleController(0);
}

int getIsIdleModeActive() {
    return idleModeActive;
}

void setShouldShuffleController(int toValue) {
    shouldShuffleController = toValue;
    shuffleControllerCountdown = SHUFFLE_CONTROLLER_DURATION;
}

void beginCountdownToApplyBossRushRings() {
    countdownToApplyBossRushRings = 3;

    // Sonic 3D blast
    if (getActiveBossRushListing().gameIndex == 6) {
        countdownToApplyBossRushRings = 60;
    }
}

void zeroDeathCount() {
    playerDeathCount = 0;
}

int getDeathCount() {
    return playerDeathCount;
}

int getNinesChallengeElapsedFrames() {
    return ninesChallengeElapsedFrames;
}

void resetNinesChallengeElapsedTimer() {
    ninesChallengeElapsedFrames = 0;
}


int getNinesChallengeElapsedSecs() {
    return (ninesChallengeElapsedFrames / 60) % 60;
}

int getNinesChallengeElapsedMins() {
    return (ninesChallengeElapsedFrames / 3600) % 60;
}

int getNinesChallengeElapsedHours() {
    return ninesChallengeElapsedFrames / (3600 * 60);
}


int getBossRushElapsedFrames() {
    return bossRushElapsedFrames;
}

void resetBossRushElapsedTimer() {
    bossRushElapsedFrames = 0;
}

int getBossRushElapsedSecs() {
    return (bossRushElapsedFrames / 60) % 60;
}

int getBossRushElapsedMins() {
    return (bossRushElapsedFrames / 3600) % 60;
}

int getBossRushElapsedHours() {
    return bossRushElapsedFrames / (3600 * 60);
}

void initialiseRewindRAM() {
    cartloader_initialiseRewindDirectory();
}

void cacheRewindRAM() {
    if (menuDisplay_getSecondaryHackOptions().shouldSaveRewindStates != 0) {
        cartLoader_saveRewindStateForCurrentGame();
    }
}

int stepBackRewindRAM() {
    if (menuDisplay_getSecondaryHackOptions().shouldSaveRewindStates != 0) {
        return cartLoader_loadRewindStateForCurrentGame();
    } else {
        return 0;
    }
}

void fireSnapEffect(int isFromTwitch) {
    snapEffectTime = snapEffectMaxTime;
    shuffleSnapValues(isFromTwitch);
}

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
        for (int i = 0; i < MAX_ROMS; i++) {
            postRingEffectCooldownTimePerGame[i] = 0;
        }
        for (int i = 0; i < 0x10000; i++) {
            holdDurations[i] = 0;
        }
        initialiseRewindRAM();
        cartloader_initialiseNetworkDirectories();

        layerRenderer_populateLetters();
        menuDisplay_initialise();
        cartLoader_run();
        initialiseBossRush();
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

static int bossRushStartCountDown = 0;
static int ninesChallengeStartCountDown = 0;

void dismissStartupHint(int andSave) {
    hasDismissedStartupHint = 1;

    if (andSave != 0) {
        FILE *networkOptionsWriter = fopen("_magicbox/__startupHint.data", "wb");
        fclose(networkOptionsWriter);
    }
}

void modConsole_applyHackOptions() {
    switchAfterTimeCounter = 0;
    switchAfterTimePeriod = 0;

    switchCooldownCounter = 0;
    switchCooldownPeriod = 0;

    saveAllStatesTimeCounter = 0;

    consecutiveEventCount = 0;

    if (shouldUseBossRush() == 0 && menuDisplay_getHackOptions().switchGameType == 6) {
        populateBossRushes();
    }

    if (checkForBossRushStart() == 1) {
        bossRushStartCountDown = 2;
        hasDismissedStartupHint = 1;
    }

    if (checkForNinesChallengeStart() == 1) {
        ninesChallengeStartCountDown = 2;
        hasDismissedStartupHint = 1;
    }

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
    if (menuDisplay_getHackOptions().cooldownOnSwitch == 4) {
        switchCooldownPeriod = 150;
    }
    if (menuDisplay_getHackOptions().cooldownOnSwitch == 5) {
        switchCooldownPeriod = 60 * 5;
    }
    if (menuDisplay_getHackOptions().cooldownOnSwitch == 6) {
        switchCooldownPeriod = 60 * 15;
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

    applyLayerHidingOptions();

    if (menuDisplay_getHackOptions().limitedColourType == 0) {
        vdp_setShouldLimitColourPalettes(0);
        // countdownToUnrandomiseColours = 60;
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

    aa_psg_setAllowCrunch(menuDisplay_getSecondaryHackOptions().colourDeleteAffectsAudio);
    aa_ym2612_setAllowCrunch(menuDisplay_getSecondaryHackOptions().colourDeleteAffectsAudio);
    aa_ym2413_setAllowCrunch(menuDisplay_getSecondaryHackOptions().colourDeleteAffectsAudio);
}

void applyLayerHidingOptions() {
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
}

void modConsole_applyNetworkOptions() {
    NetworkOptions networkOptions = menuDisplay_getNetworkOptions();

    if (networkOptions.networkingIsActive) {
        cartloader_initialiseNetworkDirectories();
    }
}

int modConsole_getSnapOffsetForRowIndex(int rowIndex) {
    if (snapEffectTime <= 0) {
        return 0;
    }

    int totalValue = 0;
    double relativeTime = ((double) snapEffectTime) / snapEffectMaxTime;

    for (int i = 0; i < 0x10; i++) {
        double relativeIndex = (((double)(rowIndex + snapEffectOffset[i])) / snapEffectHeight[i]);
        double floatOff = sin(relativeIndex * M_PI * 2);
        double roundedValue = round(floatOff * snapEffectWidth[i] * relativeTime);
        totalValue += (int) roundedValue;
    }

    return totalValue;
}

void shuffleSnapValues(int isFromTwitch) {
    for (int i = 0; i < 0x10; i++) {
        snapEffectHeight[i] = (rand() % 100) + 10;
        snapEffectWidth[i] = (rand() % 3) + 1;
        if (isFromTwitch != 0) {
            snapEffectHeight[i] *= 4;
            snapEffectWidth[i] *= 4;
        }
        snapEffectOffset[i] = (rand() % 100);
    }
}

int pendingRingTriggerShouldFire() {
    if (pendingRingTriggers > 0 && pendingRingTriggerTimer == 1) {
        return 1;
    }
    return 0;
}

void updatePendingRingTrigger() {
    if (pendingRingTriggers > 0) {
        pendingRingTriggerTimer--;
        if (pendingRingTriggerTimer <= 0) {
            pendingRingTriggers--;
            pendingRingTriggerTimer = intervalBetweenPendingTriggers;
        }
    }
    hasFlaggedPendingRingsThisFrame = 0;
}

void increasePendingRingTriggers(int count) {
    if (hasFlaggedPendingRingsThisFrame == 0) {
        pendingRingTriggers += count;
        hasFlaggedPendingRingsThisFrame = 1;
        pendingRingTriggerTimer = intervalBetweenPendingTriggers;
    }
}

int checkForBossDefeats() {
    BossRushChallengeListing listing = getActiveBossRushListing();

    int defeatLoc = listing.defeatedByte;
    // char rushTextEnd[0x40];
    // sprintf(rushTextEnd, "%04X %02X - %02X", defeatLoc, aa_genesis_getWorkRam(defeatLoc), listing.defeatedValue);
    // layerRenderer_fill(2, 0, vdp_getScreenHeight() - 16, 8 * 12, 8, 0xFF);
    // layerRenderer_writeWord256(2, 0, vdp_getScreenHeight() - 16, rushTextEnd, 0x5);

    int wasGameOver = 0;
    if (aa_genesis_getWorkRam(defeatLoc) == listing.defeatedValue
       || (listing.defeatedValue >= 0x100 && aa_genesis_getWorkRam(defeatLoc) > 0)) {
        wasGameOver = 1;
    }

    if (wasGameOver == 1) {
        onBossDefeated();
        return 1;
    }
    return 0;
}

void cacheBossRushRingCount() {
    BossRushChallengeListing listing = getActiveBossRushListing();

    // Sonic 3D Blast nullifies ring count at odd points so I account for it here
    if (listing.gameIndex == 6 && countdownToApplyBossRushRings == 0) {
        AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();
        // cache ring count every frame, unless:
        //  (1) ring count is reset to 0, and Sonic is NOT damaged
        if (aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[0]) > 0 || aa_genesis_getWorkRam(0xC224) > 0xE0) {
            // char debugText[0x40];
            // sprintf(debugText, "will cache rings %02X %02X", 
            //     aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[0]), 
            //     aa_genesis_getWorkRam(0xC224));
            // cartLoader_appendToLog(debugText);
            cacheRingCountInBossRush(1);
        }
    }

    if (listing.gameIndex != 6 && countdownToApplyBossRushRings == 0) {
        cacheRingCountInBossRush(1);
    }
}

void fireEventOnBossHit(int asNetwork) {
    if (asNetwork == 1) {
        sendNetworkMessageOnHitBoss();
    } else {
        promptSwitchGame();
    }
}

void checkForAllBossHitsInCurrentGame() {
    // cartLoader_appendToLog("-- checkForAllBossHitsInCurrentGame --");
    int currentGameId = cartLoader_getActiveCartIndex();
    for (int i = 0; i < MAX_ROMS; i++) {
        dangerouslySetActiveBossRushSlotId(i);
        BossRushChallengeListing listing = getBossRushChallengeWithIndex(i);

        if (listing.gameIndex != 0) {
            // char debugLog[0x100];
            // sprintf(debugLog, "  boss %i (game %i zone % i act %i)", i, listing.gameIndex, listing.zoneIndex, listing.actIndex);

            if (listing.gameIndex == currentGameId) {
                // sprintf(debugLog, "%s CHECKING", debugLog);

                checkForBossHits(0, i);

                // // stop if we've switched game!
                if (cartLoader_getActiveCartIndex() != currentGameId) {
                    // break;
                    // sprintf(debugLog, "%s CHECKING - SWITCHED!", debugLog);
                }
            }
            // cartLoader_appendToLog(debugLog);
        }
    }
}

void checkForBossHits(int asNetwork, int challengeIndex) {
    BossRushChallengeListing listing = getActiveBossRushListing();
    if (challengeIndex >= 0) {
        listing = getBossRushChallengeWithIndex(challengeIndex);
    }
    

    int indexX = 0;
    int indexY = 0;

    int foundCount = 0;

    int SHOW_DEBUG = 0;
    int FORCE_QUICK_KILLS = 0;
    
    if (SHOW_DEBUG == 1) {
        char activeText[0x40];
        sprintf(activeText, "%i %i .. %i %i %i %i", getActiveBossRushIndex(), getActiveBossRushSlotId(), getBossRushIndexInSlot(0), getBossRushIndexInSlot(1), getBossRushIndexInSlot(2), getBossRushIndexInSlot(3));
        layerRenderer_fill(2, 0, vdp_getScreenHeight() - 24, 8 * 24, 8, 0xFF);
        layerRenderer_writeWord256(2, 0, vdp_getScreenHeight() - 24, activeText, 0x5);

        char activeText2[0x40];
        sprintf(activeText2, "%02X %02X %02X %02X", listing.objectIdNumbers[0], listing.objectIdNumbers[1], listing.objectIdNumbers[2], listing.objectIdNumbers[3]);
        layerRenderer_fill(2, 0, vdp_getScreenHeight() - 32, 8 * 24, 8, 0xFF);
        layerRenderer_writeWord256(2, 0, vdp_getScreenHeight() - 32, activeText2, 0x5);
    }

    int objStep = 1;
    if (listing.objectIdsArePointers != 0) {
        objStep = 4;
    }

    // right now this is just used in Sonic 3D Blast
    if (listing.objectLocationStart == listing.objectLocationEnd) {
        int locationToCheck = listing.objectLocationStart;
        if (aa_genesis_getWorkRam(locationToCheck) != aa_genesis_getLastWorkRam(locationToCheck)
            && aa_genesis_getWorkRam(locationToCheck) != 0
            && aa_genesis_getLastWorkRam(locationToCheck) != 0) {
            // account for the fact that the ring count is zeroed before the switch in Sonic 3D
            // cacheRingCountInBossRush(1);

            fireEventOnBossHit(asNetwork);
            fireScreenSnapOnEvent();
        }        

        // right now this is just used in Panic Puppet
        for (int i = 0; i < 0x20; i++) {
            int index = listing.additionalHealthByteLocations[i];
            if (index > 0) {
                if (aa_genesis_getWorkRam(index) != aa_genesis_getLastWorkRam(index)
                    && aa_genesis_getWorkRam(index) != 0
                    && aa_genesis_getLastWorkRam(index) != 0) {
                    // account for the fact that the ring count is zeroed before the switch in Sonic 3D
                    // cacheRingCountInBossRush(1);

                    fireEventOnBossHit(asNetwork);
                    fireScreenSnapOnEvent();
                }     
            }
        }

        // // DEBUG!!
        // if (aa_genesis_getWorkRam(locationToCheck) > 2) {
        //     aa_genesis_setWorkRam(locationToCheck, 2);
        // }
    }

    // char listingDebug[0x100];
    // sprintf(listingDebug, "    [%04X, %04X, %04X] (%02X, %02X, %02X, %02X)",
    //     listing.objectLocationStart, listing.objectLocationEnd, listing.objectLocationSize,
    //     listing.objectIdNumbers[0], listing.objectIdNumbers[1], listing.objectIdNumbers[2], listing.objectIdNumbers[3]);

    for (int i = listing.objectLocationStart; i < listing.objectLocationEnd; i += listing.objectLocationSize) {
        int indexToCheck = i;

        for (int objectIdx = 0; objectIdx < 0x20; objectIdx += objStep) {
            int isPopulated = 0;
            if (listing.objectIdsArePointers == 0) {
                if (listing.objectIdNumbers[objectIdx] > 0) {
                    isPopulated = 1;
                }
            } else {
                for (int j = 0; j < 4; j++) {
                    if (listing.objectIdNumbers[objectIdx + j] > 0) {
                        isPopulated = 1;
                    }
                }
            }

            if (isPopulated == 1) {
                int objectFoundHere = 0;

                if (listing.objectIdsArePointers == 0) {
                    // layerRenderer_fill(2, 20, 10, 8 * 2, 8, 0xFF);

                    // sonic 1 and 2: 1-byte object IDs

                    if (aa_genesis_getWorkRam(indexToCheck) == listing.objectIdNumbers[objectIdx]) {
                        objectFoundHere = 1;

                        // sprintf(listingDebug, "%s found at %04X", listingDebug, indexToCheck);
                    } else {
                        objectFoundHere = 0;
                    }
                } else {
                    // layerRenderer_fill(2, 10, 20, 8, 8 * 5, 0xFF);

                    // sonic 3 and K: 4-byte object pointers

                    // unsigned int valueHere = 0;
                    // valueHere += ((unsigned int)aa_genesis_getWorkRam(indexToCheck + 0) * 0x1000000);
                    // valueHere += ((unsigned int)aa_genesis_getWorkRam(indexToCheck + 1) * 0x10000);
                    // valueHere += ((unsigned int)aa_genesis_getWorkRam(indexToCheck + 2) * 0x100);
                    // valueHere += ((unsigned int)aa_genesis_getWorkRam(indexToCheck + 3) * 0x1);
                    // char tempLog2[256];
                    // sprintf(tempLog2,"Checking for value at %04X ... %08X", i, valueHere);
                    // cartLoader_appendToLog(tempLog2);

                    if (aa_genesis_getWorkRam(indexToCheck + 0) ==  listing.objectIdNumbers[objectIdx + 0]
                        && aa_genesis_getWorkRam(indexToCheck + 1) ==  listing.objectIdNumbers[objectIdx + 1]
                        && aa_genesis_getWorkRam(indexToCheck + 2) ==  listing.objectIdNumbers[objectIdx + 2]
                        && aa_genesis_getWorkRam(indexToCheck + 3) ==  listing.objectIdNumbers[objectIdx + 3]) {
                        objectFoundHere = 1;
                        // sprintf(listingDebug, "%s found at %04X", listingDebug, indexToCheck);
                    } else {
                        objectFoundHere = 0;
                    }
                }

                if (objectFoundHere == 1) {
                    // this is a key value! check if it has changed!
                    int locationToCheck = indexToCheck + listing.healthByteOffsets[objectIdx];
                    if (aa_genesis_getWorkRam(locationToCheck) != aa_genesis_getLastWorkRam(locationToCheck)
                        && aa_genesis_getWorkRam(locationToCheck) != 0
                        && aa_genesis_getLastWorkRam(locationToCheck) != 0) {
                        fireEventOnBossHit(asNetwork);
                        fireScreenSnapOnEvent();
                    }

                    // for testing - quick kills!
                    if (aa_genesis_getWorkRam(locationToCheck) > 2 && FORCE_QUICK_KILLS == 1){
                         aa_genesis_setWorkRam(locationToCheck, 2);
                    }

                    if (SHOW_DEBUG == 1) {
                        char rushText[0x40];
                        sprintf(rushText, "%04X %02X", locationToCheck, aa_genesis_getWorkRam(locationToCheck));
                        layerRenderer_fill(2, 8 * 8 * foundCount, 0, 8 * 7, 8, 0xFF);
                        layerRenderer_writeWord256(2, 8 * 8 * foundCount, 0, rushText, 0x5);
                        foundCount++;

                        for (int loc = 0; loc < 0x40; loc++) {
                            char rushText2[0x40];
                            sprintf(rushText2, "%02X", aa_genesis_getWorkRam(i + loc));
                            layerRenderer_fill(2, 8 * indexX * 3, 8 * (indexY + 1), 8 * 2, 8, 0xFF);
                            layerRenderer_writeWord256(2, 8 * indexX * 3, 8 * (indexY + 1), rushText2, 0x5);

                            indexY++;
                            if (indexY >= 0x10) {
                                indexY = 0;
                                indexX++;
                            }
                        }

                        indexX++;
                    }
                }
            }
        }
    }

    // cartLoader_appendToLog(listingDebug);
}

void resetRotorValues() {
    for (int i = 0; i < 8; i++) {
        terminalRotorValues[i] = 0;
    }
}

unsigned int ramLocationsChangedByRotor[0x10000];
unsigned int ramOriginalValuesChangedByRotor[0x10000];
int rotorRamChangeLength = 0;
int rotorRamChangeDirection = 0;

unsigned int vramLocationsChangedByRotor[0x10000];
unsigned int vramOriginalValuesChangedByRotor[0x10000];
int rotorVramChangeLength = 0;
int rotorVramChangeDirection = 0;

int RAM_CHANGE_COUNT_PER_TURN = 20;
int VRAM_CHANGE_COUNT_PER_TURN = 500;

void resetRotorRam() {
    rotorRamChangeLength = 0;
    rotorRamChangeDirection = 0;
    rotorVramChangeLength = 0;
    rotorVramChangeDirection = 0;
}

void resetRotorChanges() {
    resetRotorRam();
    vdp_healAllColours();
    vdp_resetColourCycle();
}

static int rotorGameSwitchCooldown = 0;
static int rotorEditCooldown = 0;

static int lastChangeRamLoc;
static int lastChangeRamVal;

void checkRotorValues() {
    if (rotorGameSwitchCooldown > 0) {
        rotorGameSwitchCooldown--;
    }

    // for debounce
    if (rotorEditCooldown > 0) {
        rotorEditCooldown --;
    }

    for (int i = 0; i < 8; i++) {
        if (terminalRotorValues[i] != 0) {
            if (rotorEditCooldown <= 0) {
                sprintf(lastTerminalEffect, "Unknown effect");
                reportToLED("2");
            }


            // do something specific to this rotator
            if (i == 1 && rotorEditCooldown <= 0) {
                rotorEditCooldown = 3;
                for (int repeat = 0; repeat < RAM_CHANGE_COUNT_PER_TURN; repeat++) {
                    // corrupt/uncorrupt RAM
                    if (rotorRamChangeDirection == 0) {
                        rotorRamChangeDirection = 1;
                        if (terminalRotorValues[i] < 0) {
                            rotorRamChangeDirection = -1;
                        }
                    }

                    int direction = 1;
                    if (terminalRotorValues[i] < 0) {
                        direction = -1;
                    }
                    int shouldIncrement = 0;
                    if (direction == rotorRamChangeDirection || rotorRamChangeLength <= 0) {
                        shouldIncrement = 1;
                    }

                    if (shouldIncrement) {
                        // sprintf(lastTerminalEffect, "Edit RAM (%i - %i - %i) %i (%04X %02X)", terminalRotorValues[i], direction, rotorRamChangeDirection, rotorRamChangeLength, lastChangeRamLoc, lastChangeRamVal);
                        sprintf(lastTerminalEffect, "Edit RAM");

                        // make a change
                        int loc = getBigRandomNumber(0x10000);
                        int val = rand() % 0x100;
                        ramLocationsChangedByRotor[rotorRamChangeLength % 0x10000] = loc;
                        ramOriginalValuesChangedByRotor[rotorRamChangeLength % 0x10000] = aa_genesis_getWorkRam(loc);
                        aa_genesis_setWorkRam(loc, val);

                        rotorRamChangeLength++;
                        rotorRamChangeDirection = direction;

                        lastChangeRamLoc = loc;
                        lastChangeRamVal = val;
                    } else {
                        // sprintf(lastTerminalEffect, "Reverse RAM edits (%i - %i) %i", direction, rotorRamChangeDirection, rotorRamChangeLength);
                        sprintf(lastTerminalEffect, "Reverse RAM edits");

                        // reverse a change
                        if (rotorRamChangeLength > 0) {
                            rotorRamChangeLength--;
                            aa_genesis_setWorkRam(
                                ramLocationsChangedByRotor[rotorRamChangeLength % 0x10000],
                                ramOriginalValuesChangedByRotor[rotorRamChangeLength % 0x10000]
                            );
                        }
                    }
                }
            }
            
            if (i == 2 && rotorEditCooldown <= 0) {
                rotorEditCooldown = 3;
                for (int repeat = 0; repeat < VRAM_CHANGE_COUNT_PER_TURN; repeat++) {

                    // corrupt/uncorrupt VRAM
                    if (rotorVramChangeDirection == 0) {
                        rotorVramChangeDirection = 1;
                        if (terminalRotorValues[i] < 0) {
                            rotorVramChangeDirection = -1;
                        }
                    }

                    int direction = abs(terminalRotorValues[i]) / terminalRotorValues[i];
                    int shouldIncrement = 0;
                    if (direction == rotorVramChangeDirection || rotorVramChangeLength <= 0) {
                        shouldIncrement = 1;
                    }

                    if (shouldIncrement) {
                        sprintf(lastTerminalEffect, "Edit video RAM");

                        // make a change
                        int loc = getBigRandomNumber(0x10000);
                        int val = rand() % 0x100;
                        vramLocationsChangedByRotor[rotorRamChangeLength % 0x10000] = loc;
                        vramOriginalValuesChangedByRotor[rotorRamChangeLength % 0x10000] = aa_genesis_getVRamValue(loc);
                        aa_genesis_setVRamValue(loc, val);

                        rotorVramChangeLength++;
                        rotorVramChangeDirection = direction;
                    } else {
                        sprintf(lastTerminalEffect, "Reverse video RAM edits");

                        // reverse a change
                        if (rotorVramChangeLength > 0) {
                            rotorVramChangeLength--;
                            aa_genesis_setVRamValue(
                                vramLocationsChangedByRotor[rotorRamChangeLength % 0x10000],
                                vramOriginalValuesChangedByRotor[rotorRamChangeLength % 0x10000]
                            );
                        }
                    }
                }
            }

            if (i == 3 && rotorEditCooldown <= 0) {
                rotorEditCooldown = 3;
                vdp_incrementColourCycleAmount(terminalRotorValues[i]);
                sprintf(lastTerminalEffect, "Cycle colours");
            }
            
            if (i == 4) {
                if (shouldUseBossRush()) {
                    sprintf(lastTerminalEffect, "Switch boss");
                } else {
                    sprintf(lastTerminalEffect, "Switch game");
                }

                if (rotorGameSwitchCooldown <= 0) {
                    if (shouldUseBossRush()) {
                        bumpToNextBossRush();
                    } else {
                        switchToRandomAllowedGame();
                        cartLoader_loadCurrentStartupStateFromDisk();
                    }
                    rotorGameSwitchCooldown = 30;
                }
            }

            if (i == 5 && rotorEditCooldown <= 0) {
                rotorEditCooldown = 3;
                // add/remove colours!
                if (terminalRotorValues[i] < 0) {
                    sprintf(lastTerminalEffect, "Remove colours");
                    for (int i = 0; i < 3; i++) {
                        vdp_reduceColours();
                    }
                } else {
                    sprintf(lastTerminalEffect, "Put colours back");
                    for (int i = 0; i < 3; i++) {
                        vdp_healReducedColour();
                    }
                }
            }

            idleModeFrameCount = 0;
            fireSnapEffect(0);

            terminalEffectDisplayCountdown = TERMINAL_EFFECT_DISPLAY_DURATION;
        }
    }

    resetRotorValues();
}

void deductFromRingCount(int amount) {
    AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();
    
    char deductLog1[0x100];
    sprintf(deductLog1, "deductFromRingCount: %i");
    cartLoader_appendToLog(deductLog1);

    for (int i = 0; i < amount; i++) {
        int lowByte = gameTransferListing.ringBytesForTransfer[0];
        int highByte = gameTransferListing.ringBytesForTransfer[1];

        int total = (highByte * 0x100) + lowByte;
        if (gameTransferListing.ringCalculatationType == 1) {
            int convertedHigh = ((highByte / 0x10) * 10) + (highByte % 10);
            int convertedLow = ((lowByte / 0x10) * 10) + (lowByte % 10);
            total = (convertedHigh * 100) + convertedLow;
        }

        char deductLog2[0x100];
        sprintf(deductLog2, "   lowByte: %02X, highByte: %02X, total: %02X", lowByte, highByte, total);
        cartLoader_appendToLog(deductLog2);


        if (total > 0) {
            total--;
        }

        int newHighByte = total / 0x100;
        int newLowByte = total % 0x100;

        if (gameTransferListing.ringCalculatationType == 1) {
            int thousands = (total / 1000) % 10;
            int hundreds = (total / 100) % 10;
            int tens = (total / 10) % 10;
            int units = (total) % 10;

            newHighByte = (thousands * 0x10) + hundreds;
            newLowByte = (tens * 0x10) + units;
        }

        char deductLog3[0x100];
        sprintf(deductLog3, "    -> lowByte: %02X, highByte: %02X, total: %02X", newLowByte, newHighByte, total);
        cartLoader_appendToLog(deductLog3);

        
        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[0], newLowByte);
        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[1], newHighByte);
    }
    cacheRingCountInBossRush(1);
}

void modConsole_updateFrame() {
    lastPadState = padState;
    padState = input.pad[0]; //reverseOutcomeOfControlShuffling(input.pad[0]);

    if (snapEffectTime > 0) {
        snapEffectTime--;
    }

    NetworkOptions networkOptions = menuDisplay_getNetworkOptions();
    if (networkOptions.networkingIsActive != 0) {
        sendQueuedNetworkMessage();
        cartLoader_checkNetworkForActions();
    }

    if (menuDisplay_isShowing() != 0) {
        resetRotorValues();

        if (pendingRingTriggers > 0) {
            pendingRingTriggerTimer--;
        }

        // vdp_clearGraphicLayer(2);

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

        menuDisplay_onUpdate();

        rewindFrameCounter = 0;
    } else {
        layerRenderer_clearLayer(3);
        checkDeathCounter();
        applyHeldValues();

        // DEBUG - activate sonic 3 level select
        // if (aa_genesis_getWorkRam(0xF601) == 0x0C) {
        //     aa_genesis_setWorkRam(0xFFD0, 1);
        //     aa_genesis_setWorkRam(0xFFD1, 1);
        //     aa_genesis_setWorkRam(0xFFD2, 1);
        //     aa_genesis_setWorkRam(0xFFD3, 1);
        // }


        // // // Below: how to halt music in Sonic 2
        // if (debug_haltMusicCountdown > 0) {
        //     // set the "stop all sounds" flag
        //     aa_genesis_setZ80Ram(0x1B88, 0);
        //     // set the tempo to 0
        //     aa_genesis_setZ80Ram(0x1B82, 0);
        //     // when we load new music its tempo will not be 0
        //     // so we can check "oh the music has changed!"
        //     // and fire this flag
        //     debug_haltMusicCountdown--;
        // }

        // if (aa_genesis_getZ80Ram(0x1B82) != debug_lastMusicTempo) {
        //     debug_lastMusicTempo = aa_genesis_getZ80Ram(0x1B82);
        //     debug_haltMusicCountdown = 10;
        // }

        // writeWRAMintoLevelLayout();
        // writeWRAMintoSpriteBuffer();

        int cartIndex = cartLoader_getActiveCartIndex();
        if (postRingEffectCooldownTimePerGame[cartIndex] > 0) {
            postRingEffectCooldownTimePerGame[cartIndex]--;
        }
        int ringCountChangedThisFrame = ringCountHasChanged(1);
        if (ringCountChangedThisFrame) {
            reportToLED("1");
        }

        if (shouldShuffleController != 0) {
            shuffleControllerCountdown--;
            if (shuffleControllerCountdown <= 0) {
                gamepad_shuffleControls();
                shuffleControllerCountdown = SHUFFLE_CONTROLLER_DURATION;
                fireSnapEffect(1);
                showShuffleAlertCountdown = SHOW_SHUFFLE_ALERT_DURATION;
                reportToLED("3");
            }
        }
        if (showShuffleAlertCountdown > 0) {
            showShuffleAlertCountdown--;
        }

        // write nonsense once per frame
        // for (int i = 0; i < 1; i++) {
        //     aa_genesis_setWorkRam(rand() % 0x10000, rand() % 0x100);
        // }

        /*
        if (ringCountChangedThisFrame != 0) {
            vramWriteOffset += rand() % 0x100;
        }
        vdp_writeWRAMintoVRAM(vramWriteOffset);
        */

        rewindFrameCounter++;
        if (rewindFrameCounter >= framesBetweenRewindCache) {
            rewindFrameCounter = 0;
            cacheRewindRAM();
        }

        networkMessageLength = 0;
        for (int i = 0; i < 0x100; i++) {
            queuedNetworkMessage[i] = 0;
        }

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

        // colour effects should also come before switching so they don't get lost
        if (hackOpts.colourDeleteTrigger == 1) {
            removeColourOnRing(1);
        } else if (hackOpts.colourDeleteTrigger == 2) {
            removeColourOnRing(10);
        } else if (hackOpts.colourDeleteTrigger == 3) {
            removeColourTimer++;
            if (removeColourTimer >= 6) {
                vdp_reduceColours();
                removeColourTimer = 0;
            }
        } else if (hackOpts.colourDeleteTrigger == 4) {
            removeColourTimer++;
            if (removeColourTimer >= 60) {
                vdp_reduceColours();
                removeColourTimer = 0;
            }
        } else if (hackOpts.colourDeleteTrigger == 5) {
            removeColourTimer++;
            if (removeColourTimer >= 60 * 10) {
                vdp_reduceColours();
                removeColourTimer = 0;
            }
        }

        if (hackOpts.colourDeleteHealRate == 3) {
            healColoursOnRing(1);
        } else if (hackOpts.colourDeleteHealRate == 4) {
            healColoursOnRing(5);
        } else if (hackOpts.colourDeleteHealRate == 5) {
            healColoursOnRing(10);
        } else if (hackOpts.colourDeleteHealRate <= 2) {
            healColoursByTime();
        }

        if (menuDisplay_getSecondaryHackOptions().colourDeleteAffectsAudio == 1) {
            aa_psg_setCrunchProbability(vdp_getTotalRemovedColours());
            aa_ym2612_setCrunchProbability(vdp_getTotalRemovedColours());
            aa_ym2413_setCrunchProbability(vdp_getTotalRemovedColours());
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

        checkToHaltMusic();

         if (shouldUseNinesChallenge()) {

            // ADD READ/WRITE RING COUNT HERE
            if (getNinesChallengeComplete() == 0) {
                AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();
                NinesChallengeGameParameters ninesParams = getActiveNinesChallengeGameParameters();
                NinesChallengeStageListing ninesStage = getCurrentNinesChallengeStage();
                NinesChallengeOptions ninesOptions = menuDisplay_getNinesChallengeOptions();

                // check for opponent being ahead!
                if (ninesOptions.useOnlineRace) {
                    int opponentLead = getOpponentNinesChallengeLead();
                    if (opponentLead > 0) {
                        ninesOpponentLeadCountDown--;
                        if (ninesOpponentLeadCountDown <= 0) {
                            deductFromRingCount(opponentLead);
                            ninesOpponentLeadCountDown = ninesOpponentLeadCoundDownDuration;
                        }
                    } else {
                        ninesOpponentLeadCountDown = ninesOpponentLeadCoundDownDuration;
                    }

                    if (frameCount % 60 == 0) {
                        char action[0x80];
                        sprintf(action, "%iv", getBossRushRingCarryTotal());
                        cartLoader_writeActionToNetwork(action);
                    }
                }

                int ringsWentToZero = 0;
                if (diedThisFrame) {
                    if (ninesOptions.allowTacticalDeaths == 0) {
                        enforceBumpToSameNinesStageAgain();

                        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[0], 0);
                        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[1], 0);
                        cacheRingCountInBossRush(1);

                        ringsWentToZero = 1;
                    }
                }
                
                ninesChallengeElapsedFrames++;

                int damageBoostIndex = ninesParams.damageBoostLocation;
                int damageBoostMaximum = ninesParams.damageBoostMaximum;
                

                // update the "do I play life jingle" flags
                if (ninesParams.lifeUpFlaggedLocation > 0) {
                    aa_genesis_setWorkRam(ninesParams.lifeUpFlaggedLocation, 0xFF);
                }

                // debug - bonus rings!!
                // if (buttonStateAtIndex(INPUT_INDEX_A) != 0) { 
                //     aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[0], 0xFF);
                // }

                if (countdownToApplyBossRushRings > 0) {
                    // only count down once level is loaded! (except sonic 3D blast which has a 1-sec lead time)
                    applyBossRushCachedRings();
                    if (aa_genesis_getWorkRam(0xF601) == 0x0C || ninesStage.gameId == 6) {
                        countdownToApplyBossRushRings--;
                        // layerRenderer_fill(3, 100, 18, 100, 2, 0xFF);
                    }
                } else if (aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[0]) > 0 
                    || aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[1]) > 0
                    || aa_genesis_getWorkRam(damageBoostIndex) > damageBoostMaximum) {
                    cacheRingCountInBossRush(1);
                    // layerRenderer_fill(3, 0, 18, 300, 2, 0xFF);

                    if (getBossRushRingCarryTotal() == 0) {
                        char tempLog[256];
                        sprintf(tempLog,"Rings went to zero %04X = %02X, %04X = %02X, %04X = %02X > %02X", 
                            gameTransferListing.ringBytesForTransfer[0], aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[0]),
                            gameTransferListing.ringBytesForTransfer[1], aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[1]),
                            damageBoostIndex, aa_genesis_getWorkRam(damageBoostIndex), damageBoostMaximum);
                        cartLoader_appendToLog(tempLog);


                        ringsWentToZero = 1;
                    }
                }
    
                // ADD CHECK FOR END OF LEVEL HERE

                // this check will need to be different in Sonic 3D blast
                int levelSwitchIndex = ninesParams.resetStageFlagLocation;
                unsigned int levelEndLocation = ninesParams.levelCompleteLocation;
                unsigned int levelEndValue = ninesParams.levelCompleteValue;

                // char tempLog[256];
                // sprintf(tempLog,"--> (%04X - %02X) (%04X - %02X / %02X) - ", 
                //     levelSwitchIndex, aa_genesis_getWorkRam(levelSwitchIndex),
                //     levelEndLocation, aa_genesis_getWorkRam(levelEndLocation), levelEndValue);
                // cartLoader_appendToLog(tempLog);

                // char tempLog2[256];
                // sprintf(tempLog2,"     %04X %04X / %02X %02X / %02X %02X - %i", 
                //     gameTransferListing.ringBytesForTransfer[0], gameTransferListing.ringBytesForTransfer[1],
                //     aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[0]), aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[1]),
                //     getBossRushRingCarryValues(0), getBossRushRingCarryValues(1), getBossRushRingCarryValues(2));
                // cartLoader_appendToLog(tempLog2);

                if (countdownToApplyBossRushRings == 0) {
                    // layerRenderer_fill(3, 0, 0, 300, 16, 0xFF);
                    // layerRenderer_writeWord256(3, 0, 0, tempLog, 0x5);
                    // layerRenderer_writeWord256(3, 0, 8, tempLog2, 0x5);

                    // bump level in 3D blast if you die
                    if (countdownToApplyBossRushRings == 0 && ninesOptions.allowTacticalDeaths == 1 && ninesStage.gameId == 6) {
                        if (diedThisFrame) {
                            bumpNinesChallengeLevel();
                        }
                    }
                        

                    // check for load trigger changed
                    if ((aa_genesis_getWorkRam(levelSwitchIndex) >= 0x80 && levelSwitchIndex > 0
                        && aa_genesis_getWorkRam(levelSwitchIndex) != aa_genesis_getLastWorkRam(levelSwitchIndex))
                        // or if we go to the end credits in Sonic 3
                        || (ninesStage.gameId == 3 && aa_genesis_getWorkRam(levelSwitchIndex) == 0x20)
                        // or if we go to sonic 3/K speical stage
                        || (ninesStage.gameId == 3 && aa_genesis_getWorkRam(levelSwitchIndex) == 0x34)
                        || (ninesStage.gameId == 4 && aa_genesis_getWorkRam(levelSwitchIndex) == 0x34)
                        /*s2 and s1 special stage*/
                        || (ninesStage.gameId == 2 && aa_genesis_getWorkRam(levelSwitchIndex) == 0x10)
                        || (ninesStage.gameId == 1 && aa_genesis_getWorkRam(levelSwitchIndex) == 0x10)) {
                        
                        bumpNinesChallengeLevel();
                    } else {
                        // check for score totaliser spawned
                        if (levelEndValue >= 0x100) {
                            if (aa_genesis_getWorkRam(levelEndLocation) > 0) {
                                bumpNinesChallengeLevel();
                            }
                        } else {
                            if (aa_genesis_getWorkRam(levelEndLocation) == levelEndValue) {
                                bumpNinesChallengeLevel();
                            }
                        }
                    }

                    // for debug!
                    // if (buttonStateAtIndex(INPUT_INDEX_A)) {
                    //     bumpNinesChallengeLevel();
                    // }
                }

                // CHECK FOR END OF GAME!!
                if (getBossRushRingCarryTotal() >= 999) {
                    completeNinesChallenge();
                }

                if (ringsWentToZero && ninesOptions.shouldUseCheckpoints) {
                    int checkpoint = getCurrentNinesRingCheckpoint();
                    if (checkpoint > 0) {
                        int checkpointHighByte = 0;
                        int checkpointLowByte = 0;
                        if (gameTransferListing.ringCalculatationType == 0) {
                            checkpointHighByte = checkpoint / 0x100;
                            checkpointLowByte = checkpoint % 0x100;
                        } else {
                            int checkpointHighNum = checkpoint / 100;
                            int checkpointLowNum = checkpoint % 100;

                            checkpointHighByte = ((checkpointHighNum / 10) * 0x10) + (checkpointHighNum % 10);
                            checkpointLowByte = ((checkpointLowNum / 10) * 0x10) + (checkpointLowNum % 10);
                        }

                        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[0], checkpointLowByte);
                        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[1], checkpointHighByte);

                        cacheBossRushRingCount(1);

                        ringsWentToZero = 0;
                        stepBackNinesRingCheckpoint();
                    }
                }

                if (ringsWentToZero && ninesOptions.quitOnRingLoss) {
                    completeNinesChallenge();
                }
            }

        }

        if (menuDisplay_areSoloEffectsAllowed() != 0) {
            if (hackOpts.speedUpOnRing != 0) {
                updateSpeedUpOnRing();
            }
            // boss rush deals with these behaviours if toggled on
            // in the rush settings, so ignore them here if
            // we're in boss rush, lest we confuse players
            if (shouldUseBossRush() == 0 && shouldUseNinesChallenge() == 0) {
                if (hackOpts.switchGameType == 1) {
                    updateSwitchGameOnRing();
                }
                if (hackOpts.switchGameType == 5) {
                    updateSwitchGameOnLand();
                }
                if (hackOpts.switchGameType == 6) {
                    updateSwitchGameOnRing();
                    checkForAllBossHitsInCurrentGame();
                }
            }

            if (hackOpts.randomiseVelocityOnRing != 0) {
                updateRandomiseVelocityOnRing();
            }

            if (hackOpts.overwriteLevelType > 0) {
                overwriteLevelOnRing();
            }

            if (menuDisplay_getSecondaryHackOptions().ramWritesPerRing > 0) {
                applyRamEditOnRing();
            }
            if (menuDisplay_getSecondaryHackOptions().vramWritesPerRing > 0) {
                applyVramEditOnRing();
            }
        }

        sendNetworkMessageOnGetRing();


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
            valueWriteTimeCounter = 0;
        }

        checkRotorValues();

        int yOffset = 0;
        int hasShownCount = 0;
        vdp_clearGraphicLayer(2);
        if (hackOpts.shouldShowSwapCount != 0) {
            char counterText[0x40];
            sprintf(counterText, "  SWAPS: %06d", cartLoader_getSwapCount());
            int lengthOfText = lengthOfString256(counterText);

            layerRenderer_fill(2, 0, vdp_getScreenHeight() - 8 - yOffset, 8 * lengthOfText, 8, 0xFF);
            layerRenderer_writeWord256(2, 0, vdp_getScreenHeight() - 8 - yOffset, counterText, 0x5);

            hasShownCount = 1;
            yOffset += 8;
        } 
        if (hackOpts.shouldShowDeathCount != 0) {
            char counterText[0x40];
            sprintf(counterText, " DEATHS: %06d", playerDeathCount);
            int lengthOfText = lengthOfString256(counterText);

            layerRenderer_fill(2, 0, vdp_getScreenHeight() - 8 - yOffset, 8 * lengthOfText, 8, 0xFF);
            layerRenderer_writeWord256(2, 0, vdp_getScreenHeight() - 8 - yOffset, counterText, 0x5);

            hasShownCount = 1;
            yOffset += 8;
        } 
        if (hasDismissedStartupHint == 0) {

            layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2) - 4, (vdp_getScreenHeight() / 2) - 48, 8 * 23, 96, 0xFF);
            layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2), (vdp_getScreenHeight() / 2) - 44, 8 * 22, 88, 0x5);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) - 36, "** Startup Tips **", 0xFF);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) - 20, "UP + START + B", 0xFF);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) - 12, "Access hack options", 0xFF);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 4, "LEFT + START + B", 0xFF);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 12, "Rewind game", 0xFF);
            // layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 20, "** ** ** ** **", 0xFF);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 28, "** PRESS DOWN + B **", 0xFF);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 36, "TO ACKNOWLEDGE", 0xFF);
        }
        if (shouldUseBossRush() == 1) {
            if ( getBossRushComplete() == 1) {
                layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2) - 4, (vdp_getScreenHeight() / 2) - 48, 8 * 23, 96, 0xFF);
                layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2), (vdp_getScreenHeight() / 2) - 44, 8 * 22, 88, 0x5);
                layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) - 8, "BOSS RUSH COMPLETE!", 0xFF);
                layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 8, "YOUR TIME", 0xFF);

                char elapsedText[0x80];
                sprintf(elapsedText, "%02i:%02i:%02i", getBossRushElapsedHours(), getBossRushElapsedMins(), getBossRushElapsedSecs());
                layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 16, elapsedText, 0xFF);

                BossRushOptions bossRushOptions = menuDisplay_getBossRushOptions();
                char seedText[0x80];
                sprintf(seedText, "YOUR SEED: %X%X%X%X", bossRushOptions.orderSeed[0], bossRushOptions.orderSeed[1], bossRushOptions.orderSeed[2], bossRushOptions.orderSeed[3]);
                if (bossRushOptions.shouldRevealSeed) {
                    sprintf(seedText, "%s (revealed)", seedText);
                }
                if (bossRushOptions.didEditSeed) {
                    sprintf(seedText, "%s (edited)", seedText);
                }
                for (int xOff = -1; xOff <= 1; xOff++) {
                    for (int yOff = -1; yOff <= 1; yOff++) {
                        layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2) + xOff, (vdp_getScreenHeight() / 2) + 54 + yOff, seedText, 0xFF);
                    }
                }
                layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2), (vdp_getScreenHeight() / 2) + 54, seedText, 0x6);

            } else {
                if (menuDisplay_getBossRushOptions().showProgress) {
                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (9 * 8 / 2), vdp_getScreenHeight() - 12, 9 * 8, 8, 0x5);
                    char elapsedText[0x80];
                    sprintf(elapsedText, "%02i:%02i:%02i", getBossRushElapsedHours(), getBossRushElapsedMins(), getBossRushElapsedSecs());
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight()) - 8, elapsedText, 0xFF);

                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (9 * 8 / 2), vdp_getScreenHeight() - 20, 9 * 8, 8, 0x5);
                    char progressText[0x80];
                    sprintf(progressText, "%02i / %02i", getCompletedRushCount(), getEnabledRushCount());
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight()) - 16, progressText, 0xFF);
                }
            }
        }
        if (shouldUseNinesChallenge()) {
            // SHOW NINES CHALLENGE TIMER
            if ( getNinesChallengeComplete() == 1) {
                if (getBestNinesChallengeRingCount() >= 999) {
                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2) - 4, (vdp_getScreenHeight() / 2) - 48, 8 * 23, 96, 0xFF);
                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2), (vdp_getScreenHeight() / 2) - 44, 8 * 22, 88, 0x5);
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) - 8, "CHALLENGE COMPLETE!", 0xFF);
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 8, "YOUR TIME", 0xFF);

                    char elapsedText[0x80];
                    sprintf(elapsedText, "%02i:%02i:%02i", getNinesChallengeElapsedHours(), getNinesChallengeElapsedMins(), getNinesChallengeElapsedSecs());
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 16, elapsedText, 0xFF);

                    NinesChallengeOptions ninesChallengeOptions = menuDisplay_getNinesChallengeOptions();
                    char seedText[0x80];
                    sprintf(seedText, "YOUR SEED: %X%X%X%X", ninesChallengeOptions.orderSeed[0], ninesChallengeOptions.orderSeed[1], ninesChallengeOptions.orderSeed[2], ninesChallengeOptions.orderSeed[3]);
                    if (ninesChallengeOptions.shouldRevealSeed) {
                        sprintf(seedText, "%s (revealed)", seedText);
                    }
                    if (ninesChallengeOptions.didEditSeed) {
                        sprintf(seedText, "%s (edited)", seedText);
                    }
                    if (ninesChallengeOptions.receivedSeedFromOpponent) {
                        sprintf(seedText, "%s (received)", seedText);
                    }
                    if (ninesChallengeOptions.sentSeedToOpponent) {
                        sprintf(seedText, "%s (sent)", seedText);
                    }
                    for (int xOff = -1; xOff <= 1; xOff++) {
                        for (int yOff = -1; yOff <= 1; yOff++) {
                            layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2) + xOff, (vdp_getScreenHeight() / 2) + 54 + yOff, seedText, 0xFF);
                        }
                    }
                    layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2), (vdp_getScreenHeight() / 2) + 54, seedText, 0x6);
                } else {
                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2) - 4, (vdp_getScreenHeight() / 2) - 48, 8 * 23, 96, 0xFF);
                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 22 / 2), (vdp_getScreenHeight() / 2) - 44, 8 * 22, 88, 0x5);
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) - 8, "GAME OVER", 0xFF);
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 8, "YOUR BEST RING COUNT", 0xFF);

                    char elapsedText[0x80];
                    sprintf(elapsedText, "%i", getBestNinesChallengeRingCount());
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 2) + 16, elapsedText, 0xFF);

                }

                for (int xOff = -1; xOff <= 1; xOff++) {
                    for (int yOff = -1; yOff <= 1; yOff++) {
                        layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2) + xOff, (vdp_getScreenHeight() / 2) + 80 + yOff, "(Press Start to Play Again)", 0xFF);
                    }
                }
                layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2), (vdp_getScreenHeight() / 2) + 80, "(Press Start to Play Again)", 0x6);

                if (buttonStateAtIndex(INPUT_INDEX_START)) {
                    NinesChallengeOptions ninesChallengeOptions = menuDisplay_getNinesChallengeOptions();
                    if (!ninesChallengeOptions.shouldRevealSeed) {
                        shuffleNineChallengeOrderSeed();
                    }
                    beginNinesChallenge();
                }
            } else {
                if (menuDisplay_getNinesChallengeOptions().showProgress) {
                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (9 * 8 / 2), vdp_getScreenHeight() - 12, 9 * 8, 8, 0x5);
                    char elapsedText[0x80];
                    sprintf(elapsedText, "%02i:%02i:%02i", getNinesChallengeElapsedHours(), getNinesChallengeElapsedMins(), getNinesChallengeElapsedSecs());
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight()) - 8, elapsedText, 0xFF);

                    // do the below if I want to show your record highest ring count!
                    // or best time if they have a previous completion

                    layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (9 * 8 / 2), vdp_getScreenHeight() - 20, 9 * 8, 8, 0x5);

                    char canCache[0x80];
                    sprintf(canCache, " ");
                    if (canStoreNinesRingCheckpoint(getBossRushRingCarryTotal())) {
                        sprintf(canCache,"*");
                    }

                    char progressText[0x80];
                    sprintf(progressText, "%03i (%03i)", getBossRushRingCarryTotal(), getCurrentNinesRingCheckpoint());
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight()) - 16, progressText, 0xFF);
                
                    if (flashRingsToGoCountTime > 0) {
                        flashRingsToGoCountTime--;
                        if (flashRingsToGoCountTime % (flashRingsToGoPeriod * 2) < flashRingsToGoPeriod) {
                            char ringsText[0x80];
                            sprintf(ringsText, "%03i to go!", 999 - getBossRushRingCarryTotal());
                            for (int xOff = -1; xOff <= 1; xOff++) {
                                for (int yOff = -1; yOff <= 1; yOff++) {
                                    layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2) + xOff, vdp_getScreenHeight() / 2 + yOff, ringsText, 0xFF);
                                }
                            }
                            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, vdp_getScreenHeight() / 2, ringsText, 0x6);
                        }
                    }
                }

                // check for opponent being ahead!
                if (menuDisplay_getNinesChallengeOptions().useOnlineRace) {
                    int opponentLead = getOpponentNinesChallengeLead();
                    if (opponentLead > 0) {
                        char leadAlert[0x80];
                        if (ninesOpponentLeadCountDown < ninesOpponentLeadCoundDownDuration - 60) {
                            if (opponentLead == 1) {
                                sprintf(leadAlert, "Opponent ahead by %i stage", opponentLead);
                            } else {
                                sprintf(leadAlert, "Opponent ahead by %i stages", opponentLead);
                            }
                        } else {
                            if (opponentLead == 1) {
                                sprintf(leadAlert, "Lost %i ring", opponentLead);
                            } else {
                                sprintf(leadAlert, "Lost %i rings", opponentLead);
                            }
                        }

                        for (int xOff = -1; xOff <= 1; xOff++) {
                            for (int yOff = -1; yOff <= 1; yOff++) {
                                layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2) + xOff, 24 + yOff, leadAlert, 0xFF);
                            }
                        }
                        layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, 24, leadAlert, 0x6);
                    }

                    char scoreAlert[0x80];
                    sprintf(scoreAlert, "     YOU %03d - %03d OPPONENT", getBossRushRingCarryTotal(), getNinesOpponentRingCount());
                    for (int xOff = -1; xOff <= 1; xOff++) {
                        for (int yOff = -1; yOff <= 1; yOff++) {
                            layerRenderer_writeWord256Centred(2, (vdp_getScreenWidth() / 2) + xOff, 8 + yOff, scoreAlert, 0xFF);
                        }
                    }
                    layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, 8, scoreAlert, 0x6);
                }
            }
        }

        if (showShuffleAlertCountdown > 0) {
            layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 18 / 2) - 2, (vdp_getScreenHeight() / 4) - 6, 8 * 18 + 4, 12, 0xFF);
            layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 18 / 2), (vdp_getScreenHeight() / 4) - 4, 8 * 18, 8, 0x5);
            layerRenderer_writeWord256Centred(2, vdp_getScreenWidth() / 2, (vdp_getScreenHeight() / 4), "SHUFFLED BUTTONS", 0xFF);
        }
        if (shouldShuffleController != 0 && shuffleControllerCountdown < 5 * 60) {
            layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 18 / 2) - 2, (vdp_getScreenHeight() / 4) - 6, 8 * 18 + 4, 12, 0xFF);
            layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 18 / 2), (vdp_getScreenHeight() / 4) - 4, 8 * 18, 8, 0x5);
            int screenFillAmount = ((8 * 18 - 4) * shuffleControllerCountdown) / (5 * 60);
            layerRenderer_fill(2, (vdp_getScreenWidth() / 2) - (8 * 18 / 2) + 2, (vdp_getScreenHeight() / 4) - 2, screenFillAmount, 4, 0xFF);
        }

        if (holdEffectFramesLeft > 0) {
            int barSize = ((vdp_getScreenWidth() - 8) * holdEffectFramesLeft) / holdEffectDuration;
            barSize /= 8;
            barSize *= 8;
            layerRenderer_fill(2, 4, 4, barSize, 8, 0x08);
        }

        // for terminal
        int tipsYpos = vdp_getScreenHeight() - 16;
        layerRenderer_writeWord256(2, -headingTextScrollPixels, 2 + tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels - 1, 2+ tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels + 1, 2+ tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels, 0+ tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels - 1, 0+ tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels + 1, 0+ tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels - 1, 1+ tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels + 1, 1+ tipsYpos, menuDisplay_getCurrentRulesName(), 0xFF);
        layerRenderer_writeWord256(2, -headingTextScrollPixels, 1+ tipsYpos, menuDisplay_getCurrentRulesName(), headingColour);

        if (terminalEffectDisplayCountdown > 0) {
            terminalEffectDisplayCountdown--;
            int effectXpos = vdp_getScreenWidth() / 2;
            int effectYpos = vdp_getScreenHeight() / 2;
            layerRenderer_writeWord256Centred(2, effectXpos + 1, effectYpos + 1, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos + 1, effectYpos, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos + 1, effectYpos - 1, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos - 1, effectYpos + 1, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos - 1, effectYpos, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos - 1, effectYpos - 1, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos, effectYpos+ 1, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos, effectYpos - 1, lastTerminalEffect, 0xFF);
            layerRenderer_writeWord256Centred(2, effectXpos, effectYpos, lastTerminalEffect, headingColour);

        }


        if (shouldUseBossRush()) {
            if (getBossRushComplete() == 0) {
                // if (buttonStateAtIndex(INPUT_INDEX_A) != 0) {
                //     onBossDefeated();
                // }

                bossRushElapsedFrames++;
                incrementFrameCountOfActiveBossRush();
                
                if (countdownToApplyBossRushRings > 0) {
                    // only count down once level is loaded!
                    if (aa_genesis_getWorkRam(0xF601) == 0x0C || getActiveBossRushListing().gameIndex == 6) {
                        countdownToApplyBossRushRings--;
                        applyBossRushCachedRings();
                        // if (countdownToApplyBossRushRings == 0) {
                        //     applyBossRushCachedRings();
                        // }
                    }
                }
            }

            // char rushText[0x40];
            // BossRushChallengeListing listing = getActiveBossRushListing();
            // sprintf(rushText, "%02X %02X %02X %02X", listing.objectIdNumbers[0], listing.objectIdNumbers[1], listing.objectIdNumbers[2], listing.objectIdNumbers[3]);
            // layerRenderer_fill(2, 0, vdp_getScreenHeight() - 8, 8 * 20, 8, 0xFF);
            // layerRenderer_writeWord256(2, 0, vdp_getScreenHeight() - 8, rushText, 0x5);
            int defeated = checkForBossDefeats();
            if (defeated == 0) {
                int activeBossRushIndex = getActiveBossRushIndex();
                cacheBossRushRingCount();

                if (menuDisplay_getBossRushOptions().switchTriggers.networkBossHit == 1) {
                    checkForBossHits(1, -1);
                }
                if (menuDisplay_getBossRushOptions().switchTriggers.bossHit == 1) {
                    checkForBossHits(0, -1);
                } 
                if (activeBossRushIndex == getActiveBossRushIndex() && menuDisplay_getBossRushOptions().switchTriggers.ring == 1) {
                    updateSwitchGameOnRing();
                } 
                if (activeBossRushIndex == getActiveBossRushIndex() && menuDisplay_getBossRushOptions().switchTriggers.land == 1) {
                    updateSwitchGameOnLand();
                }
            }

            if (menuDisplay_getBossRushOptions().ringsOff == 1) {
                if (getActiveBossRushListing().blockRingZeroing == 0) {
                    aa_genesis_setWorkRam(activeGameListing.ringByte, 0);
                }
            }
        }

        // TO HELP WITH SONIC 3 EDITING
        /*
        if (buttonWasPressedAtIndex(INPUT_INDEX_A) != 0) {
            // aa_genesis_setWorkRam(0xFFD0, 1);
            // aa_genesis_setWorkRam(0xFFD1, 1);
            // aa_genesis_setWorkRam(0xFFD2, 1);
            // aa_genesis_setWorkRam(0xFFD3, 1);
            aa_genesis_setWorkRam(0xF601, 0xA4);
        }
        if (buttonWasPressedAtIndex(INPUT_INDEX_B) != 0) {
            aa_genesis_incrementWorkRamCompoundValueByInt(0xB010, 2, 0x80);
            // aa_genesis_incrementWorkRamCompoundValueByInt(0xFE2A,1, 1);
            // aa_genesis_incrementWorkRamCompoundValueByInt(0xFE2B,1, 1);
            // // aa_genesis_setWorkRam(0xFE2A, 0xFF);
            // // aa_genesis_setWorkRam(0xFE2B, 0xFF);
            // aa_genesis_setWorkRam(0xF601, 0x8C);
        }
        */

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
            vdp_clearGraphicLayer(2);

            dismissStartupHint(1);
        } else if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_C) != 0) {

            cartLoader_cacheSaveStateBeforeMenu();
            menuDisplay_showMenu(MENU_LISTING_IN_GAME);
            vdp_clearGraphicLayer(2);

            dismissStartupHint(1);
        } else if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_A) != 0) {

            cartLoader_cacheSaveStateBeforeMenu();
            menuDisplay_showMenu(MENU_LISTING_IN_GAME);
            vdp_clearGraphicLayer(2);

            dismissStartupHint(1);
        } else if (
            // insta-kill!!
            buttonStateAtIndex(INPUT_INDEX_DOWN) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_B) != 0)
        {
            modConsole_activatePanic();
        } else if (
            // show the terminal!!
            buttonStateAtIndex(INPUT_INDEX_DOWN) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_A) != 0)
        {
            // menuDisplay_showTerminalMenu();
        } else if (
            buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_LEFT) != 0
        ) {
            // FOR ALISTAIR
            // saveSaveStateForCurrentGame();
            // cartLoader_saveAllSaveStatesToDisk();
        }

        if (buttonStateAtIndex(INPUT_INDEX_DOWN) != 0 && buttonStateAtIndex(INPUT_INDEX_B) != 0 && hasDismissedStartupHint == 0) {
            dismissStartupHint(0);
        }
        
        if (
            // rewind!
            (buttonStateAtIndex(INPUT_INDEX_LEFT) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_B) != 0) || shouldRewind == 1)
        {
            showRewindSymbol();
            if (framesHeldDownRewindButtons % 30 == 0) {
                int rewindSuccess = stepBackRewindRAM();
                rewindSymbolColour = (rand() % 0x20) + 1;
                if (rewindSuccess == 0) {
                    rewindSymbolColour = 0;
                    shouldRewind = 0;
                }
            }
            framesHeldDownRewindButtons ++;
        } else {
            if (framesHeldDownRewindButtons > 0) {
                hideRewindSymbol();
            }
            framesHeldDownRewindButtons = 0;
        }


        // Puyo games need to wait a few frames after a ring effect before activating another,
        // otherwise the game-end countdown counts as many rings!
        // This must occur before anything that uses ring count
        cartIndex = cartLoader_getActiveCartIndex();
        if (cartLoader_getActiveGameListing().postRingEffectCooldown > 0) {
            // some complicated wrangling so that every successive ring in that time resets the counter back to max
            if (ringCountChangedThisFrame != 0) {
                postRingEffectCooldownTimePerGame[cartIndex] = cartLoader_getActiveGameListing().postRingEffectCooldown;
                cartLoader_appendToLog("Got ring during cooldown");
            }        

            char logMsgRing[0x100];
            sprintf(logMsgRing, "postRingEffectCooldownTimePerGame[%i] = %i", cartIndex, postRingEffectCooldownTimePerGame[cartIndex]);
            cartLoader_appendToLog(logMsgRing);
        }

        // // show what buttons are being pressed!
        // char controlsTextBuf[0x40];
        // sprintf(controlsTextBuf, "%i %i %i", pendingRingTriggers, pendingRingTriggerTimer, pendingRingTriggerShouldFire());
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

        headingTextScrollSubpixels++;
        if (headingTextScrollSubpixels > HEADING_TEXT_SUBPIXEL_PER_PIXEL) {
            headingTextScrollPixels++;
            headingTextScrollSubpixels = 0;
            if (headingTextScrollPixels > (130 * 8)) {
                headingTextScrollPixels = -vdp_getScreenWidth();
                headingColour = (rand() % 10) + 5;
            }
        }



        if (shouldCheckForIdleMode) {
            idleModeFrameCount++;

            if (buttonStateAtIndex(INPUT_INDEX_LEFT) != 0 
                || buttonStateAtIndex(INPUT_INDEX_RIGHT) != 0
                || buttonStateAtIndex(INPUT_INDEX_UP) != 0
                || buttonStateAtIndex(INPUT_INDEX_DOWN) != 0
                || buttonStateAtIndex(INPUT_INDEX_START) != 0
                || buttonStateAtIndex(INPUT_INDEX_A) != 0
                || buttonStateAtIndex(INPUT_INDEX_B) != 0
                || buttonStateAtIndex(INPUT_INDEX_C) != 0) 
            {
                idleModeFrameCount = 0;
            }

            if (idleModeFrameCount > MAX_FRAMES_FOR_IDLE_MODE) {
                beginIdleMode();
                idleModeCooldown = 0;
            }
        }

        if (idleModeActive) {
            if (idleModeCooldown > 0) {
                idleModeCooldown--;
            } else {
                if (buttonStateAtIndex(INPUT_INDEX_LEFT) != 0 
                    || buttonStateAtIndex(INPUT_INDEX_RIGHT) != 0
                    || buttonStateAtIndex(INPUT_INDEX_UP) != 0
                    || buttonStateAtIndex(INPUT_INDEX_DOWN) != 0
                    || buttonStateAtIndex(INPUT_INDEX_START) != 0
                    || buttonStateAtIndex(INPUT_INDEX_A) != 0
                    || buttonStateAtIndex(INPUT_INDEX_B) != 0
                    || buttonStateAtIndex(INPUT_INDEX_C) != 0) 
                {
                    menuDisplay_showTerminalMenu();
                    idleModeActive = 0;
                }
            }
        }

        if (countdownToUnrandomiseColours > 0) {
            countdownToUnrandomiseColours--;
            if (countdownToUnrandomiseColours <= 0) {
                vdp_setShouldRandomiseColours(0);
            }
        }

        if (
            // bump into idle mode!!
            buttonStateAtIndex(INPUT_INDEX_RIGHT) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
            buttonStateAtIndex(INPUT_INDEX_A) != 0)
        {
            beginIdleMode();
            idleModeCooldown = 60;
        }


        // game switching needs to come at the end for per-game cooldown to work
        if (hackOpts.switchGameType > 1 && hackOpts.switchGameType < 5) {
            switchAfterTimeCounter++;
            if (switchAfterTimeCounter >= switchAfterTimePeriod) {
                switchAfterTimeCounter = 0;
                promptSwitchGame();
            }
        }

        if (countdownUntilRingSwitch > 0) {
            countdownUntilRingSwitch--;
            if (countdownUntilRingSwitch == 0) {
                bumpEventCountForSwitchGame();
            }
        }

        if (countdownUntilUnpause > 0) {
            countdownUntilUnpause --;
            if (countdownUntilUnpause == 0) {
                unpauseGame();
            }
        }

        if (countdownToSummonMenu > 0) {
            countdownToSummonMenu--;
            if (countdownToSummonMenu == 0) {
                menuDisplay_showMenu(MENU_LISTING_IN_GAME);
                vdp_clearGraphicLayer(2);
            }
        }

        updatePendingRingTrigger();

        if (bossRushStartCountDown > 0) {
            bossRushStartCountDown--;
            if (bossRushStartCountDown == 0) {
                beginBossRush();
            }
        }

        if (ninesChallengeStartCountDown > 0) {
            ninesChallengeStartCountDown--;
            if (ninesChallengeStartCountDown == 0) {
                beginNinesChallenge();
            }
        }

        vdp_resetCachedM5();
    }

    frameCount++;

    aa_genesis_updateLastRam();
} 

void modConsole_beginRewindAction() {
    shouldRewind = 1;
}

void modConsole_endRewindAction() {
    shouldRewind = 0;
}


void writeWRAMintoSpriteBuffer() {
    // values correct for Sonic games
    int min = 0xF800;
    int max = 0xFA80;
    int length = max - min;

    for (int i = min; i < max; i++) {
        work_ram[i] = 0;
    }

    for (int i = 0; i < 0x10000; i++) {
        if (i < min || i >= max) {
            int index = min + (i % length);
            unsigned int ramValue = work_ram[index];
            ramValue += work_ram[i];
            ramValue = ramValue % 0x100;
            work_ram[index] = (unsigned char)ramValue;
        }
    }
}

void writeWRAMintoLevelLayout() {
    // values correct for Sonic games
    int min = cartLoader_getActiveLevelEditListing().startByte;
    int max = cartLoader_getActiveLevelEditListing().endByte;
    int length = max - min;
    
    for (int i = min; i < max; i++) {
        work_ram[i] = 0;
    }

    for (int i = 0; i < 0x10000; i++) {
        if (i < min || i >= max) {
            int index = min + (i % length);
            int ramValue = work_ram[index];
            ramValue += work_ram[i];
            ramValue %= 0x100;
            work_ram[index] = (char)ramValue;
        }
    }
}

void applyRamEditOnRing() {
    if (ringCountHasChanged(0) != 0) {
        int editCount = 0;
        SecondaryHackOptions options = menuDisplay_getSecondaryHackOptions();
        if (options.ramWritesPerRing == 1) {
            editCount = 1;
        }
        if (options.ramWritesPerRing == 2) {
            editCount = 5;
        }
        if (options.ramWritesPerRing == 3) {
            editCount = 25;
        }
        if (options.ramWritesPerRing == 4) {
            editCount = 100;
        }

        // char logMsg[0x100];
        // sprintf(logMsg, "RAM ON RING will fire %i times", editCount);
        // cartLoader_appendToLog(logMsg);

        if (editCount > 0) {
            fireScreenSnapOnEvent();
            int startLoc = 0;
            int endLoc = 0;

            int multiplicand = 1;
            for (int i = 3; i >= 0; i--) {
                startLoc += options.ramWriteStartLoc[i] * multiplicand;
                endLoc += options.ramWriteEndLoc[i] * multiplicand;
                multiplicand *= 0x10;
            }
            int distance = abs(startLoc - endLoc);

            for (int i = 0; i < editCount; i++) {
                int location = startLoc;
                if (endLoc < startLoc) {
                    location = endLoc;
                }
                if (distance > 0) {
                    location += getBigRandomNumber(distance + 1);
                }

                int value = rand() % 0x100;
                aa_genesis_setWorkRam(location, value);
                // ensure this doesn't fire any ring/life trackers
                aa_genesis_setLastWorkRam(location, value);

                // char logMsg2[0x100];
                // sprintf(logMsg2, "RAM ON RING: Wrote %02X to %04X", value, location);
                // cartLoader_appendToLog(logMsg2);
            }
        }
    }
}

void applyVramEditOnRing() {
    if (ringCountHasChanged(0) != 0) {
        int editCount = 0;
        SecondaryHackOptions options = menuDisplay_getSecondaryHackOptions();
        if (options.vramWritesPerRing == 1) {
            editCount = 1;
        }
        if (options.vramWritesPerRing == 2) {
            editCount = 5;
        }
        if (options.vramWritesPerRing == 3) {
            editCount = 25;
        }
        if (options.vramWritesPerRing == 4) {
            editCount = 100;
        }

        // char logMsg[0x100];
        // sprintf(logMsg, "RAM ON RING will fire %i times", editCount);
        // cartLoader_appendToLog(logMsg);

        if (editCount > 0) {
            fireScreenSnapOnEvent();
            for (int i = 0; i < editCount; i++) {
                int location = getBigRandomNumber(0xFFFF);
                int value = rand() % 0x100;
                aa_genesis_setVRamValue(location, value);
            }
        }
    }
}

void applyHeldValues() {
    holdEffectFramesLeft = 0;
    for (int i = 0; i < 0x10000; i++) {
        if (holdDurations[i] > 0) {
            holdDurations[i]--;
            aa_genesis_setWorkRam(i, holdValues[i]);

            // char logMsg[0x100];
            // sprintf(logMsg, "APPLYING HELD VALUE: %04X at %02X (%i frames left)", i, holdValues[i], holdDurations[i]);
            // cartLoader_appendToLog(logMsg);

            if (holdEffectFramesLeft < holdDurations[i]) {
                holdEffectFramesLeft = holdDurations[i];
            }
        }
    }
}

void showRewindSymbol() {
    int midX = bitmap.viewport.w / 2;
    int midY = bitmap.viewport.h / 2;

    int startX = midX - 40;
    int startY = midY - 20;
    for (int i = 0; i < 40; i++) {
        layerRenderer_fill(2, startX + i, startY + (40 - i), 1, i * 2, rewindSymbolColour);
        layerRenderer_fill(2, startX + 40 + i, startY + (40 - i), 1, i * 2, rewindSymbolColour);
    }

    if (menuDisplay_getSecondaryHackOptions().shouldSaveRewindStates == 0) {
        layerRenderer_fill(2, midX - 110, midY - 50, 220, 100, 0xFF);
        layerRenderer_fill(2, midX - 100, midY - 40, 200, 80, 0x5);
        layerRenderer_writeWord256Centred(2, midX, midY-24, "Rewind switched off", 0xFF);

        layerRenderer_writeWord256Centred(2, midX, midY-8, "Use Quality of Life menu", 0xFF);
        layerRenderer_writeWord256Centred(2, midX, midY, "to switch it on", 0xFF);

        layerRenderer_writeWord256Centred(2, midX, midY+16, "(*UP + START + B*, then", 0xFF);
        layerRenderer_writeWord256Centred(2, midX, midY+24, "*hack options*, then", 0xFF);
        layerRenderer_writeWord256Centred(2, midX, midY+32, "*quality of life*)", 0xFF);

    }
}

void hideRewindSymbol() {
    layerRenderer_clearLayer(2);
}

void healColoursByTime() {
    healColourTimer++;

    int difficultyMultiplier = 1;
    int framesForHeal = 300;
    int spacing = 1;
    if (menuDisplay_getHackOptions().colourDeleteHealRate == 0) {
        // easy
        spacing = 4;
    }
    if (menuDisplay_getHackOptions().colourDeleteHealRate == 1) {
        // medium
        spacing = 8;
    }
    if (menuDisplay_getHackOptions().colourDeleteHealRate == 2) {
        // hard
        spacing = 16;
    }

    int totalLostColours = vdp_getTotalRemovedColours();
    for (int i = 0; i < 0xFF; i += spacing) {
        if (totalLostColours > spacing * i) {
            framesForHeal /= 2;
        }
    }

    if (healColourTimer > framesForHeal) {
        vdp_healReducedColour();
        healColourTimer = 0;
    }
}

void fireScreenSnapOnEvent() {
    SecondaryHackOptions options = menuDisplay_getSecondaryHackOptions();
    if (options.screenSnapOnGetRing != 0) {
        fireSnapEffect(0);
    }
}

void healColoursOnRing(int count) {
    if (ringCountHasChanged(0) != 0) {
        for (int i = 0; i < count; i++) {
            vdp_healReducedColour();
            fireScreenSnapOnEvent();
        }
    }
}

void removeColourOnRing(int count) {
    if (ringCountHasChanged(0) != 0) {
        for (int i = 0; i < count; i++) {
            vdp_reduceColours();
            fireScreenSnapOnEvent();
        }
    }
}

void queueNetworkMessage(char eventId) {
    queuedNetworkMessage[networkMessageLength] = eventId;
    networkMessageLength++;
}

void sendQueuedNetworkMessage() {
    if (networkMessageLength > 0) {
        cartLoader_writeActionToNetwork(queuedNetworkMessage);
    }
}

// deals with the fact that the rand() function only goes up to 0x7FFF
// the largest integer possible is 0x7FFFFFFF
int getBigRandomNumber(int maxValue) {
    int runningNumber = rand() % 0x8;
    for (int i = 0; i < 7; i++) {
        runningNumber *= 0x10;
        runningNumber += rand() % 0x10;
    }
    return runningNumber % maxValue;
}

void modConsole_processNetworkEvent(char eventId, int eventCount, int eventLocation, int eventDistance, int isFromTwitch, int holdDuration) {
    // do a SNAP effect ONLY if character actually matches an effect

    if (eventId == NETWORK_MSG_SWITCH_GAME) {
        fireSnapEffect(isFromTwitch);
        int switchingIsAllowed = 1;

        // check in case we're at the game over phase of a challenge
        if (shouldUseBossRush() && getBossRushComplete()) {
            switchingIsAllowed = 0;
        }
        if (shouldUseNinesChallenge() && getNinesChallengeComplete()) {
            switchingIsAllowed = 0;
        }

        // check in case we're in a gamestate where switching game would be dangerous/annoying
        // (e.g. in a menu)
        AAGameTransferListing transfer = cartLoader_getActiveGameTransferListing();
        if (transfer.gameStateByte != 0) {
            unsigned int value = aa_genesis_getWorkRam(transfer.gameStateByte);
            for (int i = 0; i < 0x10; i++) {
                if (transfer.gameStatesToBlockSwitch[i] >= 0) {
                    //the top bit is used for something in Sonic 3
                    if (value % 0x80 == transfer.gameStatesToBlockSwitch[i]) {
                        switchingIsAllowed = 0;
                    }
                }
            }
        }
        if (switchingIsAllowed > 0) {
            cartLoader_appendToLog("Switching game from network");
            promptSwitchGame();
        } else {
            cartLoader_appendToLog("Switching game from network (blocked!)");
        }
    }

    if (eventId == NETWORK_MSG_SPEED_UP) {
        fireSnapEffect(isFromTwitch);
        if (cartLoader_getActiveGameListing().accelerationType == 1) {
            cartLoader_appendToLog("Increasing Sonic 2D speed from network");
            for (int i = 0; i < eventCount; i++) {
                aa_genesis_incrementWorkRamCompoundValueByInt(0xF760, 2, 0x40);
                aa_genesis_incrementWorkRamCompoundValueByInt(0xF762, 2, 0x08);
            }
        }
    }

    if (eventId == NETWORK_MSG_RANDOMISE_VELOCITY) {
        fireSnapEffect(isFromTwitch);
        cartLoader_appendToLog("Randomising velocity from network");
        applyRandomiseVelocity();
    }

    int scrambleLevelCount = 0;
    if (eventId == NETWORK_MSG_SCRAMBLE_LEVEL_EASY) {
        scrambleLevelCount = 1; // was 10
    }
    if (eventId == NETWORK_MSG_SCRAMBLE_LEVEL_MEDIUM) {
        scrambleLevelCount = 2; // was 20
    }
    if (eventId == NETWORK_MSG_SCRAMBLE_LEVEL_HARD) {
        scrambleLevelCount = 5; // was 50
    }
    if (scrambleLevelCount > 0) {
        fireSnapEffect(isFromTwitch);
        cartLoader_appendToLog("Scrambling level from network");
        overwriteLevel(scrambleLevelCount * eventCount, 1);
    }

    if (eventId == NETWORK_MSG_REMOVE_COLOUR) {
        fireSnapEffect(isFromTwitch);
        for (int i = 0; i < eventCount; i++) {
            vdp_reduceColours();
        }
    }
    if (eventId == NETWORK_MSG_REMOVE_10_COLOURS) {
        fireSnapEffect(isFromTwitch);
        for (int i = 0; i < 10; i++) {
            vdp_reduceColours();
        }
    }

    if (eventId == NETWORK_MSG_TOGGLE_LAYER) {
        fireSnapEffect(isFromTwitch);
        char logMsg[0x100];
        sprintf(logMsg, "Toggle visble layers: %i", eventLocation);
        cartLoader_appendToLog(logMsg);

        if (eventLocation == 100) {
            menuDisplay_showAllVisibleLayers();
            cartLoader_appendToLog("Shown all visible");
        } else {
            menuDisplay_toggleVisibleLayers();
            cartLoader_appendToLog("Toggled");
        }
        applyLayerHidingOptions();
    }

    if (eventId == NETWORK_MSG_WRITE_TO_RAM) {
        fireSnapEffect(isFromTwitch);
        int maxValue = 0x10000;
        if (cartLoader_consoleForCurrentCart() == CART_TYPE_MASTERSYSTEM || cartLoader_consoleForCurrentCart() == CART_TYPE_GAMEGEAR) {
            maxValue = 0x2000;
            // cartLoader_appendToLog("Is MS or GG so max is 0x1FFF");
        } else {
            // cartLoader_appendToLog("Is MD so max is 0xFFFF");
        }
        for (int i = 0; i < eventCount; i++) {
            int index = getBigRandomNumber(maxValue);
            int value = rand() % 0x100;
            // char logMsg[0x100];
            // sprintf(logMsg, "setting byte %04X to %02X", index, value);
            // cartLoader_appendToLog(logMsg);

            aa_genesis_setWorkRam(index, value);
        }
    }

    if (eventId == NETWORK_MSG_WRITE_TO_CART) {
        fireSnapEffect(isFromTwitch);
        for (int i = 0; i < eventCount; i++) {
            int index = rand() % MAXROMSIZE;
            setCartValueAtIndex(index, rand() % 0x100);
        }
    }

    if (eventId == NETWORK_MSG_WRITE_SPECIFIC_TO_RAM) {
        fireSnapEffect(isFromTwitch);
        if (eventDistance == 0) {
            int valueToWrite = eventCount;
            if (eventCount >= 0x100) {
                valueToWrite = rand() % 0x100;
            }
            aa_genesis_setWorkRam(eventLocation, valueToWrite);
        } else {
            int cartSize = 0x10000;
            if (cartLoader_consoleForCurrentCart() == CART_TYPE_MASTERSYSTEM || cartLoader_consoleForCurrentCart() == CART_TYPE_GAMEGEAR) {
                cartSize = 0x2000;
            }
            for (int i = 0; i < eventDistance; i++) {
                int valueToWrite = eventCount;
                if (eventCount >= 0x100) {
                    valueToWrite = rand() % 0x100;
                }
                int location = (eventLocation + i) % cartSize;
                if (holdDuration == 0) {
                    char logMsg[0x100];
                    sprintf(logMsg, "SETTING %04X at %02X", location, valueToWrite);
                    cartLoader_appendToLog(logMsg);

                    aa_genesis_setWorkRam(location, valueToWrite);
                } else {
                    // char logMsg[0x100];
                    // sprintf(logMsg, "HOLDING %04X at %02X for %i frames", location, valueToWrite, holdDuration);
                    // cartLoader_appendToLog(logMsg);

                    holdDurations[location] = holdDuration;
                    holdValues[location] = valueToWrite;

                    holdEffectDuration = holdDuration;
                }
            }
        }
    }

    if (eventId == NETWORK_MSG_HEAL_COLOURS) {
        vdp_healAllColours();
    }
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
    if (ringCountHasChanged(0) != 0) {
        HackOptions hackOpts = menuDisplay_getHackOptions();
        int cycleCount = 10;
        if (hackOpts.overwriteLevelDifficulty == 1) {
            cycleCount = 20;
        }
        if (hackOpts.overwriteLevelDifficulty == 2) {
            cycleCount = 50;
        }

        overwriteLevel(cycleCount, hackOpts.overwriteLevelType);

        fireScreenSnapOnEvent();
    }
}

void sendNetworkMessageOnHitBoss() {
    if (menuDisplay_getBossRushOptions().switchTriggers.networkBossHit) {
        queueNetworkMessage(NETWORK_MSG_SWITCH_GAME);
    }
}

void sendNetworkMessageOnGetRing() {
    if (ringCountHasChanged(0) != 0) {
        NetworkOptions networkOpts = menuDisplay_getNetworkOptions();
        if (networkOpts.networkingIsActive != 0) {
            queueNetworkMessage(NETWORK_MSG_DUMMY_TWTICH_MESSAGE);

            if (networkOpts.sendSwitchGame != 0) {
                queueNetworkMessage(NETWORK_MSG_SWITCH_GAME);
            }
            if (networkOpts.sendSpeedUp != 0) {
                queueNetworkMessage(NETWORK_MSG_SPEED_UP);
            }
            if (networkOpts.sendRandomiseVelocity != 0) {
                queueNetworkMessage(NETWORK_MSG_RANDOMISE_VELOCITY);
            }

            if (networkOpts.sendWriteIntoLevelDifficulty == 1) {
                queueNetworkMessage(NETWORK_MSG_SCRAMBLE_LEVEL_EASY);
            }
            if (networkOpts.sendWriteIntoLevelDifficulty == 2) {
                queueNetworkMessage(NETWORK_MSG_SCRAMBLE_LEVEL_MEDIUM);
            }
            if (networkOpts.sendWriteIntoLevelDifficulty == 3) {
                queueNetworkMessage(NETWORK_MSG_SCRAMBLE_LEVEL_HARD);
            }

            if (networkOpts.sendRemoveColour == 1) {
                queueNetworkMessage(NETWORK_MSG_REMOVE_COLOUR);
            }
            if (networkOpts.sendRemoveColour == 2) {
                queueNetworkMessage(NETWORK_MSG_REMOVE_10_COLOURS);
            }
        }
    }
}

void overwriteLevel(int cycleCount, int overwriteType) {
    // check in case we're in a gamestate where scrambling would be dangerous
    AAGameTransferListing transfer = cartLoader_getActiveGameTransferListing();
    if (transfer.gameStateByte != 0) {
        unsigned int value = aa_genesis_getWorkRam(transfer.gameStateByte);
        // char logMsg[0x100];
        // sprintf(logMsg, "Value at transfer.gameStateByte %04X is %02X", transfer.gameStateByte, value);
        // cartLoader_appendToLog(logMsg);

        for (int i = 0; i < 0x10; i++) {
            if (transfer.gameStatesToBlockScramble[i] >= 0) {
                // char logMsg2[0x100];
                // sprintf(logMsg2, "Checking transfer.gameStatesToBlockScramble[%i] valye %02X", i, transfer.gameStatesToBlockScramble[i]);
                // cartLoader_appendToLog(logMsg2);
                //the top bit is used for something in Sonic 3
                if (value % 0x80 == transfer.gameStatesToBlockScramble[i]) {
                    // cartLoader_appendToLog("Found this value as a SKIP_ME");
                    return;
                }
            }
        }
        // cartLoader_appendToLog("Did NOT find this value as a SKIP_ME");
    } else {
        // char logMsg[0x100];
        // sprintf(logMsg, "transfer.gameStateByte is zero");
        // cartLoader_appendToLog(logMsg);
    }

    // If we got to here, it's safe!
    AALevelEditListing levelEdits = cartLoader_getActiveLevelEditListing();
    if (levelEdits.endByte > 0 && levelEdits.endByte > levelEdits.startByte) {
        for (int i = 0; i < cycleCount; i++) {
            unsigned int value = 0;
            if (overwriteType == 1) {
                value = rand() % 0x100;
            }
            unsigned int index = (rand() % (levelEdits.endByte - levelEdits.startByte)) + levelEdits.startByte;
            aa_genesis_setWorkRam(index, value);
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

    if (shouldUseBossRush()) {
        bumpToNextBossRush();
    } else if(shouldUseNinesChallenge()) {
        bumpNinesChallengeLevel();
    } else {
        cartLoader_loadRandomRom();
    }
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

void checkDeathCounter() {
    diedThisFrame = 0;
    int shouldIncrement = 0;

    // this uses the assumption that bytes 0 and 1 are a life counter, and byte 2 is an "update plz" trigger so we shouldn't track it
    for (int i = 0; i < 2; i++) {
        if (cartLoader_getActiveGameListing().livesBytes[i] != 0) {
            int lastVal = aa_genesis_getLastWorkRam(cartLoader_getActiveGameListing().livesBytes[i]);
            int nowVal = aa_genesis_getWorkRam(cartLoader_getActiveGameListing().livesBytes[i]);
            if (lastVal - nowVal == 1) {
                char logMsg[0x100];
                sprintf(logMsg, "counted death %i (%04X): lastVal %02X, nowVal %02X", i, cartLoader_getActiveGameListing().livesBytes[i], lastVal, nowVal);
                cartLoader_appendToLog(logMsg);
                shouldIncrement = 1;
            }
        }
    }

    if (shouldIncrement != 0) {
        playerDeathCount++;
        reportToLED("4");
        diedThisFrame = 1;
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
            // ensure this doesn't trigger the death counter
            // by changing the LAST work ram too!
            aa_genesis_setLastWorkRam(activeGameListing.timeBytes[i], activeGameListing.timeByteDestinations[i]);
        } else {
            break;
        }
    }
}

void modConsole_activatePanic() {
    // for ALISTAIR
    // saveSaveStateForCurrentGame();
    // cartLoader_saveAllSaveStatesToDisk();

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

    vdp_healAllColours();

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

void applyRandomiseVelocity() {
    // cartLoader_appendToLog("Will apply random velocity");
    MomentumControlListing momentumDef = cartLoader_getMomentumControlListing();
    if (momentumDef.radius > 0) {
        // cartLoader_appendToLog("Am happy with radius");
        double angle = ((double)(rand() % 360) / 360) * M_PI * 2;
        int xVel = abs((int)(momentumDef.radius * sin(angle)));
        int yVel = abs((int)(momentumDef.radius * cos(angle)));

        int inertiaDiff = 0;
        if (momentumDef.inertiaMax > momentumDef.inertiaMin) {
            inertiaDiff = (rand() % (momentumDef.inertiaMin - momentumDef.inertiaMax));
        }
        if (momentumDef.inertiaMax < momentumDef.inertiaMin) {
            inertiaDiff = (rand() % (momentumDef.inertiaMax - momentumDef.inertiaMin));
        }
        int inertia = momentumDef.inertiaMin + inertiaDiff;

        if (rand() % 100 < 50 && xVel > 0) {
            xVel = 0x100 - xVel;
        }
        if (rand() % 100 < 50 && yVel > 0) {
            yVel = 0x100 - yVel;
        }
        if (aa_genesis_getWorkRam(momentumDef.inertiaByte) > 0) {
            // player is moving
            // if player is going right, force them to go left
            if (aa_genesis_getWorkRam(momentumDef.inertiaByte) < 128 && inertia > 0) {
                inertia = 0x100 - inertia;
            }
        } else {
            // player is standing still
            // set inertia to a random direction
            if (rand() % 100 < 50 && inertia > 0) {
                inertia = 0x100 - inertia;
            }
        }

        // char logMessage1[0x100];
        // sprintf(logMessage1, "angle: %0.4f, xVel: %02X, yVel %02X, inertia %02X", angle, xVel, yVel, inertia);
        // cartLoader_appendToLog(logMessage1);

        aa_genesis_setWorkRam(momentumDef.xByteStart, xVel);
        aa_genesis_setWorkRam(momentumDef.inertiaByte, inertia);
        aa_genesis_setWorkRam(momentumDef.yByteStart, yVel);
    } else {
        // cartLoader_appendToLog("Did not apply random velocity because of radius");
    }
}

void updateRandomiseVelocityOnRing() {
    if (ringCountHasChanged(0) != 0) {
        applyRandomiseVelocity();
        fireScreenSnapOnEvent();
    }
}

void forceSonicSpeed(unsigned int amount) {
    //speed
    aa_genesis_setWorkRam(0xF760, amount % 0x100);
    aa_genesis_setWorkRam(0xF761, amount / 0x100);

    //acceleration
    unsigned int divisor = 128; // matches in-game speed
    unsigned int acceleration = amount / divisor;
    aa_genesis_setWorkRam(0xF762, acceleration % 0x100);
    aa_genesis_setWorkRam(0xF763, acceleration / 0x100);
}

void updateSpeedUpOnRing() {
    if (ringCountHasChanged(0) != 0) {
        if (cartLoader_getActiveGameListing().accelerationType == 1) {
            // fireSnapEffect();

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

        fireScreenSnapOnEvent();
    }
}

void updateSwitchGameOnLand() {
    if (switchCooldownCounter > 0) {
        switchCooldownCounter --;
    }


    if ( switchCooldownCounter <= 0) {
        // AAStandTriggerListing triggers = cartLoader_getActiveStandTriggerListing();
        // if (cartLoader_getActiveStandTriggerListing().standingCooldown > 0) { // standingCooldown doesn't work as I expect...
            // || triggers.standingByte == 0) { // ... so I also check "is this a non-standing game!"
        if (standingHasChanged(0) != 0) {
            bumpEventCountForSwitchGame();
        }

        // account for pixel games if no standing byte declared
        AAStandTriggerListing triggers = cartLoader_getActiveStandTriggerListing();
        if (switchAfterTimeCounter <= 0 && triggers.standingByte == 0) {
            cartLoader_checkPixelTrackerForStateChange();
        }
    }
}

void bumpEventCountForSwitchGame() {
    consecutiveEventCount++;
    int requiredEvents = 1;
    int eventCountFlag = menuDisplay_getSecondaryHackOptions().eventCountForSwitch;
    if (eventCountFlag == 1) {
        requiredEvents = 2;
    }
    if (eventCountFlag == 2) {
        requiredEvents = 5;
    }
    if (eventCountFlag == 3) {
        requiredEvents = 10;
    }

    if (consecutiveEventCount >= requiredEvents) {
        consecutiveEventCount = 0;
        promptSwitchGame();
        fireScreenSnapOnEvent();   
    } 
}

void updateSwitchGameOnRing() {
    if (switchCooldownCounter > 0) {
        switchCooldownCounter --;
    }

    if (switchCooldownCounter <= 0) {
        if (ringCountHasChanged(0) != 0) {
            // layerRenderer_clearLayer(0);
            // char word[0x20];
            // sprintf(word, "%d", aa_genesis_getWorkRam(activeGameListing.ringByte));

            // layerRenderer_fill(0, 0, 0, 32, 8, 1);
            // layerRenderer_writeWord256(0, 0, 0, word, 5);
            if (activeGameListing.ringSwitchCooldown > 0) {
                countdownUntilRingSwitch = activeGameListing.ringSwitchCooldown;
            } else {
                bumpEventCountForSwitchGame();
            }
        }

        if (switchAfterTimeCounter <= 0) {
            cartLoader_checkPixelTrackerForStateChange();
        }
    }
}

int standingHasChanged(int shouldIgnoreCooldown) {
    if (postRingEffectCooldownTimePerGame[cartLoader_getActiveCartIndex()] > 0 && shouldIgnoreCooldown == 0) {
        return 0;
    }

    AAStandTriggerListing triggers = cartLoader_getActiveStandTriggerListing();
    if (triggers.standingByte > 0) {
        unsigned int lastStanding = aa_genesis_getLastWorkRam(triggers.standingByte);
        unsigned int currentStanding = aa_genesis_getWorkRam(triggers.standingByte);

        int lastBitStatus = (lastStanding >> triggers.standingBit) & 1;
        int currentBitStatus = (currentStanding >> triggers.standingBit) & 1;

        // char debugWord[0x100];
        // sprintf(debugWord, "triggers %04X >> %02X, (%02X --> %02X) >> (%01X --> %01X), frame %d", triggers.standingByte, triggers.standingBit, lastStanding, currentStanding, lastBitStatus, currentBitStatus, frameCount);
        // cartLoader_appendToLog(debugWord);

        if (currentBitStatus == triggers.standingRequiredValue && currentBitStatus != lastBitStatus) {
            // char word[0x100];
            // sprintf(word, "Standing went from %02X to %02X, (%02X --> %02X), frame %d", lastBitStatus, currentBitStatus, lastStanding, currentStanding, frameCount);
            // cartLoader_appendToLog(word);
            postRingEffectCooldownTimePerGame[cartLoader_getActiveCartIndex()] = 5;
            return 1;
        }
    } else {
        // in other games (e.g. micro machines) switch on normal "ring-like" events
        return ringCountHasChanged(shouldIgnoreCooldown);
    }
    return 0;
}

int ringCountHasChanged(int shouldIgnoreCooldown) {
    if (postRingEffectCooldownTimePerGame[cartLoader_getActiveCartIndex()] > 0 && shouldIgnoreCooldown == 0) {
        return 0;
    }

    AAScoreMonitorListing scoreListing = cartLoader_getActiveScoreMonitorListing();

    if (activeGameListing.ringByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(activeGameListing.ringByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(activeGameListing.ringByte);

        if (currentRingCount != 0 && currentRingCount != lastRingCount) {
            char word[0x100];
            sprintf(word, "Ring count went from %02X to %02X, frame %d", lastRingCount, currentRingCount, frameCount);
            cartLoader_appendToLog(word);

            // e.g. if you pick up a 10 ring box, make 10 things happen
            if (scoreListing.allowStackRingInputs == 1) {
                int difference = abs((int)lastRingCount - (int)currentRingCount);
                if (difference == 10 || difference == 50) {
                    increasePendingRingTriggers(difference);
                }
            }

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
            } else if (scoreListing.calculatationType == 2) {
                // single digits in decimal (Lucky Dime Caper) - each byte is a decimal digit
                lastScore += lastScoreVal * multiplier;
                currentScore += currentScoreVal * multiplier;
                multiplier *= 10;
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
    if (scoreListing.allowNegativeChange != 0 &&
        multiplier > 1 && currentScore < lastScore - scoreListing.scoreJumpForTrigger && blockedBecauseZero == 0) {
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

            // char logMsg[0x100];
            // sprintf(logMsg, "score2 at %i: %i --> %i", i, lastScoreVal, currentScoreVal);
            // cartLoader_appendToLog(logMsg);


            //change the below for different calculation types
            lastScore += lastScoreVal * multiplier;
            currentScore += currentScoreVal * multiplier;
            multiplier *= 0x100;
        } else {
            break;
        }
    }

    // char logMsg[0x100];
    // sprintf(logMsg, "    score2: %i --> %i", lastScore, currentScore);
    // cartLoader_appendToLog(logMsg);

    blockedBecauseZero = 0;
    if (scoreListing.blockJumpFromZero != 0) {
        if (lastScore == 0) {
            blockedBecauseZero = 1;
        }
    }
    if (multiplier > 1 && currentScore > lastScore + scoreListing.scoreJumpForTrigger && blockedBecauseZero == 0) {
        return 1;
    }
    if (scoreListing.allowNegativeChange != 0 &&
        multiplier > 1 && currentScore < lastScore - scoreListing.scoreJumpForTrigger && blockedBecauseZero == 0) {
        return 1;
    }

    if (pendingRingTriggerShouldFire() == 1) {
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

void modConsole_getLockOnRomHeader(char intoArray[]) {
    if (0x201000 < cart.romsize) {
        uint8 tempHeader[0x20];
        for (int i = 0; i < 0x20; i++) {
            tempHeader[i] = 0;
        }

        uint8 byteArray[0x20];
        for (int i = 0; i < 0x20; i++) {
            int index = 0x200000 + 0x100 + 0x20 + i;
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

        for (int i = 0; i < 0x20; i++) {
            if (i <= tempIndex) {
                intoArray[i] = tidiedHeader[i];
            }
            else {
                intoArray[i] = 0;
            }
        }
    } else {
        writeStringToArray32("TOO SMALL TO LOCK-ON", intoArray);
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