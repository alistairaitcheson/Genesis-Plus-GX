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
static int ramDetectiveIndex = 0;
static int gameSwapOptionIndex = 0;
static int qualityOfLifeOptionIndex = 0;
static int saveStateOptionIndex = 0;
static int sonicSpecificOptionIndex = 0;
static int visualsOptionIndex = 0;
static int pixelDetectiveIndex = 0;
static int networkingOptionsIndex = 0;

static int majorVersion = 0;
static int minorVersion = 13;

static int DEFAULT_WIDTH = 320;
static int DEFAULT_HEIGHT = 200;

static int queuedMenu = MENU_LISTING_NONE;

static int gameHasStarted = 0;
static int saveStateWasLoaded = 0;

static HackOptions hackOptions;
static PersistValuesOptions persistValuesOptions;
static RamDetectiveOptions ramDetectiveOptions;
static PixelDetectiveOptions pixelDetectiveOptions;
static NetworkOptions networkOptions;
static int logRamStateCounter[0x10000];

static int trackedRamFrameCounts[0x10000];
// static int trackedRamLocationCount = 0;

static int maxFileNameLength = 28;

static int trackedPixelValues[8];

HackOptions menuDisplay_getHackOptions() {
    return hackOptions;
}

PersistValuesOptions menuDisplay_getPersistValuesOptions() {
    return persistValuesOptions;
}

NetworkOptions menuDisplay_getNetworkOptions() {
    return networkOptions;
}

void menuDisplay_sendNetworkOptionsToOpponent() {
    char message[0x100];
    sprintf(message, "");
    
    message[0] = NETWORK_MSG_INTERPRET_AS_RULES;

    message[1] = networkOptions.sendSwitchGame != 0 ? NETWORK_MSG_INTERPRET_AS_POSITIVE : NETWORK_MSG_INTERPRET_AS_NEGATIVE;
    message[2] = NETWORK_MSG_SWITCH_GAME;

    message[3] = networkOptions.sendSpeedUp != 0 ? NETWORK_MSG_INTERPRET_AS_POSITIVE : NETWORK_MSG_INTERPRET_AS_NEGATIVE;
    message[4] = NETWORK_MSG_SPEED_UP;
    
    message[5] = networkOptions.sendRandomiseVelocity != 0 ? NETWORK_MSG_INTERPRET_AS_POSITIVE : NETWORK_MSG_INTERPRET_AS_NEGATIVE;
    message[6] = NETWORK_MSG_RANDOMISE_VELOCITY;
        
    message[7] = networkOptions.sendWriteIntoLevelDifficulty != 0 ? NETWORK_MSG_INTERPRET_AS_POSITIVE : NETWORK_MSG_INTERPRET_AS_NEGATIVE;
    message[8] = NETWORK_MSG_SCRAMBLE_LEVEL_EASY;
    if (networkOptions.sendWriteIntoLevelDifficulty == 1) {
        message[8] = NETWORK_MSG_SCRAMBLE_LEVEL_EASY;
    }
    if (networkOptions.sendWriteIntoLevelDifficulty == 2) {
        message[8] = NETWORK_MSG_SCRAMBLE_LEVEL_MEDIUM;
    }    
    if (networkOptions.sendWriteIntoLevelDifficulty == 3) {
        message[8] = NETWORK_MSG_SCRAMBLE_LEVEL_HARD;
    }

    cartLoader_writeActionToNetwork(message);
}

void menuDisplay_applyNetworkOptionSwitch(char command, int asPositive) {
    if (command == NETWORK_MSG_SWITCH_GAME) {
        networkOptions.sendSwitchGame = asPositive; 
    }
    if (command == NETWORK_MSG_SPEED_UP) {
        networkOptions.sendSpeedUp = asPositive; 
    }
    if (command == NETWORK_MSG_RANDOMISE_VELOCITY) {
        networkOptions.sendRandomiseVelocity = asPositive; 
    }
    if (command == NETWORK_MSG_SCRAMBLE_LEVEL_EASY) {
        if (asPositive == 0) {
            networkOptions.sendWriteIntoLevelDifficulty = 0; 
        } else {
            networkOptions.sendWriteIntoLevelDifficulty = 1; 
        }
    }
    if (command == NETWORK_MSG_SCRAMBLE_LEVEL_MEDIUM) {
        if (asPositive == 0) {
            networkOptions.sendWriteIntoLevelDifficulty = 0; 
        } else {
            networkOptions.sendWriteIntoLevelDifficulty = 2; 
        }
    }
    if (command == NETWORK_MSG_SCRAMBLE_LEVEL_HARD) {
        if (asPositive == 0) {
            networkOptions.sendWriteIntoLevelDifficulty = 0; 
        } else {
            networkOptions.sendWriteIntoLevelDifficulty = 3; 
        }
    }

    networkOptions.awaitingOpponentSettingsState = 2;

    // make sure to refresh the network menu if it's showing!
    if (menuDisplay_isShowing() != 0) {
        menuDisplay_showMenu(activeMenu);
    }
}

int menuDisplay_isShowing() {
    if (activeMenu == MENU_LISTING_NONE) {
        return 0;
    } else {
        return 1;
    }
}

int menuDisplay_areSoloEffectsAllowed() {
    if (networkOptions.allowSoloEffectswhenNetworked != 0 || networkOptions.networkingIsActive == 0) {
        return 1;
    }
    return 0;
}

int menuDisplay_shouldGameSwapOptionsShowAsOn() {
    if (hackOptions.switchGameType != 0) {
        return 1;
    }
    return 0;
}

int menuDisplay_shouldQualityOfLifeOptionsShowAsOn() {
    if (hackOptions.infiniteLives != 0 || hackOptions.infiniteTime != 0 || hackOptions.shouldWriteToLog != 0) {
        return 1;
    }
    return 0;
}

int menuDisplay_shouldPersistValueOptionsShowAsOn() {
    if (persistValuesOptions.lives != 0 || 
        persistValuesOptions.momentum != 0 ||
        persistValuesOptions.rings != 0 ||
        persistValuesOptions.score != 0 ||
        persistValuesOptions.time != 0 ||
        persistValuesOptions.topSpeed != 0) {
        return 1;
    }

    return 0;
}

int menuDisplay_shouldSonicSpecificOptionsShowAsOn() {
    if (menuDisplay_areSoloEffectsAllowed() == 0) {
        return 0;
    }
    if (hackOptions.speedUpOnRing != 0 || hackOptions.randomiseVelocityOnRing != 0 || hackOptions.overwriteLevelType != 0) {
        return 1;
    }
    if (menuDisplay_shouldPersistValueOptionsShowAsOn()) {
        return 1;
    }
    return 0;
}

int menuDisplay_shouldVisualsOptionsShowAsOn() {
    if (hackOptions.shouldSortColours != 0 || hackOptions.limitedColourType != 0 || hackOptions.copyVram != 0 || hackOptions.shouldHideLayers != 0) {
        return 1;
    }
    return 0;
}

int menuDisplay_shouldNetworkingOptionsShowAsOn() {
    if (networkOptions.networkingIsActive != 0) {
        return 1;
    }
    return 0;
}

int menuDisplay_shouldSaveStateOptionsShowAsOn() {
    if (hackOptions.automaticallySaveStatesFreq != 0) {
        return 1;
    }
    return 0;
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

    applyDefaultRamDetectiveValues();

    FILE *networkValuesReader = fopen("_magicbox/__networkOptions.data", "rb");
    if (networkValuesReader) {
        int networkBuffer[0x100];
        fread(networkBuffer, sizeof(int), 0x100, networkValuesReader);
        fclose(networkValuesReader);
        applyNetworkOptionsFromArray256(networkBuffer);
    } else {
        applyNetworkOptionsDefaultValues();
    }
}

// void addRamLocationToTracker(int location) {
//     if (trackedRamLocationCount >= 0x10000){
//         return;
//     }

//     int found = 0;
//     for (int i = 0; i < trackedRamLocationCount; i++) {
//         if (trackedRamLocations[i] == location) {
//             found = 1;
//             break;
//         }
//     }

//     if (found == 0) {
//         trackedRamLocations[trackedRamLocationCount] = location;
//         trackedRamLocationCount++;
//     }
// }

void menuDisplay_updateRamDetective() {
    if (ramDetectiveOptions.shouldShow == 0) {
        return;
    }

    int start = 
        (ramDetectiveOptions.startLoc[0] * 0x1000) + 
        (ramDetectiveOptions.startLoc[1] * 0x0100) + 
        (ramDetectiveOptions.startLoc[2] * 0x0010) + 
        (ramDetectiveOptions.startLoc[3] * 0x0001);
    int end = 
        (ramDetectiveOptions.endLoc[0] * 0x1000) + 
        (ramDetectiveOptions.endLoc[1] * 0x0100) + 
        (ramDetectiveOptions.endLoc[2] * 0x0010) + 
        (ramDetectiveOptions.endLoc[3] * 0x0001);
    int seekValue = 
        (ramDetectiveOptions.seekValue[0] * 0x10) +
        (ramDetectiveOptions.seekValue[1] * 0x01);

    for (int i = start; i <= end; i++) {
        int value = aa_genesis_getWorkRam(i);
        if (value == seekValue) {
            trackedRamFrameCounts[i]++;
        } else {
            trackedRamFrameCounts[i] = 0;
        }
    }
}

void menuDisplay_renderRamDetective() {

    layerRenderer_clearLayer(0);

    int startY = 0;
    int x = 8;
    int width = (8 * 4);
    int height = 8;
    int maxHeight = vdp_getScreenHeight() - height;

    if (cartLoader_consoleForCurrentCart() == CART_TYPE_GAMEGEAR) {
        x += 40;
        startY += 30;
        maxHeight -= 100;
    }

    int showedTracker = 0;
    if (ramDetectiveOptions.shouldShowTracker != 0) {
        for (int i = 0; i < 8; i++) {
            int loc = 
                (ramDetectiveOptions.trackerLocations[i][0] * 0x1000) + 
                (ramDetectiveOptions.trackerLocations[i][1] * 0x0100) + 
                (ramDetectiveOptions.trackerLocations[i][2] * 0x0010) + 
                (ramDetectiveOptions.trackerLocations[i][3] * 0x0001);
            if (loc > 0) {
                showedTracker = 1;
                char text[0x10];

                unsigned char value = aa_genesis_getWorkRam(loc);

                sprintf(text, "%04X:%02X", loc, value);
                int localY = startY + (i * 9);
                layerRenderer_fill(0, x, localY, 7 * 8, height, 0xFF);
                layerRenderer_writeWord256(0, x, localY, text, 5);
            }
        }
    }


    if (ramDetectiveOptions.shouldShow == 0) {
        return;
    }

    if (showedTracker != 0) {
        x += 7 * 8 + 1;
    }
    int y = startY;

    for (int i = 0; i < 0x10000; i++) {
        if (trackedRamFrameCounts[i] > ramDetectiveOptions.minFrames) {
            char text[0x10];
            sprintf(text, "%04X", i);
            layerRenderer_fill(0, x, y, width, height, 0xFF);
            layerRenderer_writeWord256(0, x, y, text, 5);

            y += height + 1;
            if (y >= vdp_getScreenHeight() - height) {
                y = startY;
                x += width + 1;
            }
        }
    }
}

void menuDisplay_updatePixelDetective(int line, uint8 linebuf[2][0x200]) {
    if (pixelDetectiveOptions.shouldShow != 1) {
        return;
    }

    for (int i = 0; i < 8; i++) {
        int xPos = (pixelDetectiveOptions.coordsListings[i][0] * 0x10)
            + (pixelDetectiveOptions.coordsListings[i][1] * 0x01);
        int yPos = (pixelDetectiveOptions.coordsListings[i][2] * 0x10)
            + (pixelDetectiveOptions.coordsListings[i][3] * 0x01);

        if (xPos > 0 || yPos > 0) {
            if (yPos == line) {
                trackedPixelValues[i] = linebuf[0][0x20 + xPos];
            }
        } else {
            trackedPixelValues[i] = -1;
        }
    }
}

void menuDisplay_renderPixelDetective() {
    if (pixelDetectiveOptions.shouldShow != 1) {
        return;
    }

    layerRenderer_clearLayer(0);

    int xCoords[8];
    int yCoords[8];

    for (int i = 0; i < 8; i++) {
        int xPos = (pixelDetectiveOptions.coordsListings[i][0] * 0x10)
            + (pixelDetectiveOptions.coordsListings[i][1] * 0x01);
        int yPos = (pixelDetectiveOptions.coordsListings[i][2] * 0x10)
            + (pixelDetectiveOptions.coordsListings[i][3] * 0x01);
        xCoords[i] = xPos;
        yCoords[i] = yPos;
    }

    for (int i = 0; i < 8; i++) {
        if (trackedPixelValues[i] != -1) {
            layerRenderer_fill(0, xCoords[i]-1, yCoords[i]-1, 1, 3, 0xFF);
            layerRenderer_fill(0, xCoords[i], yCoords[i]-1, 1, 1, 0xFF);
            layerRenderer_fill(0, xCoords[i], yCoords[i]+1, 1, 1, 0xFF);
            layerRenderer_fill(0, xCoords[i]+1, yCoords[i]-1, 1, 3, 0xFF);
            char pixelText[0x20];
            sprintf(pixelText, "%02X", trackedPixelValues[i]);
            layerRenderer_fill(0, xCoords[i]+1, yCoords[i]+1, 16, 8, 0xFF);
            layerRenderer_writeWord256(0, xCoords[i], yCoords[i], pixelText, 5);
        }
    }
}

void clearLogRamState() {
    for (int i = 0; i < 0x10000; i++) {
        logRamStateCounter[i] = 0;
    }
}

void menuDisplay_logRamStateToTrackedValues() {
    cartLoader_appendToLog("*** menuDisplay_logRamStateToTrackedValues ***");
    int start = 
        (ramDetectiveOptions.startLoc[0] * 0x1000) + 
        (ramDetectiveOptions.startLoc[1] * 0x0100) + 
        (ramDetectiveOptions.startLoc[2] * 0x0010) + 
        (ramDetectiveOptions.startLoc[3] * 0x0001);
    int end = 
        (ramDetectiveOptions.endLoc[0] * 0x1000) + 
        (ramDetectiveOptions.endLoc[1] * 0x0100) + 
        (ramDetectiveOptions.endLoc[2] * 0x0010) + 
        (ramDetectiveOptions.endLoc[3] * 0x0001);
    int seekValue = 
        (ramDetectiveOptions.seekValue[0] * 0x10) +
        (ramDetectiveOptions.seekValue[1] * 0x01);

    char headingLog[0x20];
    sprintf(headingLog, "*** Seeking %02X (%04X to %04X)", seekValue, start, end);
    cartLoader_appendToLog(headingLog);

    int maxCounter = 0;
    for (int i = 0; i < 0x10000; i++) {
        if (i < start || i > end) {
            logRamStateCounter[i] = 0;
        }
        if (aa_genesis_getWorkRam(i) == seekValue) {
            logRamStateCounter[i] ++;
            char logText[0x10];
            sprintf(logText, "%04X (%i)", i, logRamStateCounter[i]);
            cartLoader_appendToLog(logText);

            if (maxCounter < logRamStateCounter[i]) {
                maxCounter = logRamStateCounter[i];
            }
        } else {
            logRamStateCounter[i] = 0;
        }
    }

    for (int i = 0; i < 0x10000; i++) {
        if (logRamStateCounter[i] == maxCounter) {
            char logText[0x20];
            sprintf(logText, "%02X: MAXIMUM %04X (%i)", seekValue, i, logRamStateCounter[i]);
            cartLoader_appendToLog(logText);            
        }
    }
}

void applyPersistValuesFromArray256(int array256[]) {
    for (int i = 0; i < 0x100; i++) {
        if (array256[i] != 0) {
            array256[i] = 1;
        }
    }

    persistValuesOptions.lives = array256[0];
    persistValuesOptions.rings = array256[1];
    persistValuesOptions.topSpeed = array256[2];
    persistValuesOptions.momentum = array256[3];
    persistValuesOptions.time = array256[4];
    persistValuesOptions.score = array256[5];
}

void applyNetworkOptionsFromArray256(int array256[]) {
    for (int i = 0; i < 0x100; i++) {
        if (array256[i] != 0 && i != 3) {
            array256[i] = 1;
        }
    }

    networkOptions.networkingIsActive = array256[0];
    networkOptions.sendSwitchGame = array256[1];
    networkOptions.sendSpeedUp = array256[2];
    networkOptions.sendWriteIntoLevelDifficulty = array256[3];
    networkOptions.sendRandomiseVelocity = array256[4];
    networkOptions.allowSoloEffectswhenNetworked = array256[5];
}

void applyDefaultPersistValues() {
    persistValuesOptions.lives = 0;
    persistValuesOptions.rings = 0;
    persistValuesOptions.topSpeed = 0;
    persistValuesOptions.momentum = 0;
    persistValuesOptions.time = 0;
    persistValuesOptions.score = 0;
}

void applyDefaultRamDetectiveValues() {
    ramDetectiveOptions.startLoc[0] = 0;
    ramDetectiveOptions.startLoc[1] = 0;
    ramDetectiveOptions.startLoc[2] = 0;
    ramDetectiveOptions.startLoc[3] = 0;

    ramDetectiveOptions.endLoc[0] = 0xF;
    ramDetectiveOptions.endLoc[1] = 0xF;
    ramDetectiveOptions.endLoc[2] = 0xF;
    ramDetectiveOptions.endLoc[3] = 0xF;

    ramDetectiveOptions.seekValue[0] = 0;
    ramDetectiveOptions.seekValue[1] = 0;

    ramDetectiveOptions.minFrames = 0;
    ramDetectiveOptions.shouldShow = 0;

    for (int i = 0; i < 0x10000; i++) {
        trackedRamFrameCounts[i] = 0;
        logRamStateCounter[i] = 0;
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            ramDetectiveOptions.trackerLocations[i][j] = 0;
        }
    }

    ramDetectiveOptions.shouldShowTracker = 0;
}

void applyNetworkOptionsDefaultValues() {
    networkOptions.networkingIsActive = 0;
    networkOptions.sendSwitchGame = 0;
    networkOptions.sendSpeedUp = 0;
    networkOptions.sendWriteIntoLevelDifficulty = 0;
    networkOptions.sendRandomiseVelocity = 0;
    networkOptions.allowSoloEffectswhenNetworked = 0;
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
    hackOptions.shouldSortColours = array256[9];
    hackOptions.limitedColourType = array256[10];
    hackOptions.shouldHideLayers = array256[11];
    hackOptions.shouldShowSwapCount = array256[12];
    hackOptions.overwriteLevelType = array256[13];
    hackOptions.overwriteLevelDifficulty = array256[14];
    hackOptions.swapOrder =  array256[15];
    hackOptions.randomiseVelocityOnRing = array256[16];
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
    hackOptions.shouldSortColours = 0;
    hackOptions.limitedColourType = 0;
    hackOptions.shouldHideLayers = 0;
    hackOptions.shouldShowSwapCount = 0;
    hackOptions.overwriteLevelType = 0;
    hackOptions.overwriteLevelDifficulty = 1;
    hackOptions.swapOrder = 0;
    hackOptions.randomiseVelocityOnRing = 0;
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
    options[9] = hackOptions.shouldSortColours;
    options[10] = hackOptions.limitedColourType;
    options[11] = hackOptions.shouldHideLayers;
    options[12] = hackOptions.shouldShowSwapCount;
    options[13] = hackOptions.overwriteLevelType;
    options[14] = hackOptions.overwriteLevelDifficulty;
    options[15] = hackOptions.swapOrder;
    options[16] = hackOptions.randomiseVelocityOnRing;

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
    persistValues[5] = persistValuesOptions.score;

    remove("_magicbox/__persistValues.data");
    FILE *persistValuesWriter = fopen("_magicbox/__persistValues.data", "wb");

    for (int i = 0; i < 0x100; i++) {
        fwrite(persistValues, sizeof(int), 0x100, persistValuesWriter);
    }
    fclose(persistValuesWriter);

    
    int networkValues[0x100];
    for (int i = 0; i < 0x100; i++) {
        networkValues[i] = 0;
    }
    networkValues[0] = networkOptions.networkingIsActive;
    networkValues[1] = networkOptions.sendSwitchGame;
    networkValues[2] = networkOptions.sendSpeedUp;
    networkValues[3] = networkOptions.sendWriteIntoLevelDifficulty;
    networkValues[4] = networkOptions.sendRandomiseVelocity;
    networkValues[5] = networkOptions.allowSoloEffectswhenNetworked;

    remove("_magicbox/__networkOptions.data");
    FILE *networkOptionsWriter = fopen("_magicbox/__networkOptions.data", "wb");
    for (int i = 0; i < 0x100; i++) {
        fwrite(networkValues, sizeof(int), 0x100, networkOptionsWriter);
    }
    fclose(networkOptionsWriter);
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

    if (activeMenu == MENU_LISTING_RAM_DETECTIVE) {
        showRamDetectiveMenu();
    }
    
    if (activeMenu == MENU_LISTING_PIXEL_DETECTIVE) {
        showPixelDetectiveMenu();
    }

    if (activeMenu == MENU_LISTING_GAME_SWAP_OPITONS) {
        showGameSwapOptionsMenu();
    }

    if (activeMenu == MENU_LISTING_QUALITY_OF_LIFE) {
        showQualityOfLifeOptionsMenu(); 
    }

    if (activeMenu == MENU_LISTING_SAVE_STATE_OPTIONS) {
        showSaveStateOptionsMenu(); 
    }

    if (activeMenu == MENU_LISTING_SONIC_SPECIFIC_OPTIONS) {
        showSonicSpecificOptionsMenu(); 
    }

    if (activeMenu == MENU_LISTING_VISUALS_OPTIONS) {
        showVisualsOptionsMenu(); 
    }

    if (activeMenu == MENU_LISTING_NETWORKING) {
        showNetworkingOptionsMenu(); 
    }
}

void menuDisplay_hideMenu() {
    activeMenu = MENU_LISTING_NONE;

    vdp_setShouldRandomiseColours(0);
    aa_psg_unmute();
    aa_ym2612_unmute();

    layerRenderer_clearLayer(0);

    if (gameHasStarted != 0 && saveStateWasLoaded == 0) {
        cartLoader_loadSaveStateForQuitMenu();
    }

    if (saveStateWasLoaded != 0) {
        // modConsole_flagToSummonMenu();
    }
    saveStateWasLoaded = 0;
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
    modConsole_applyNetworkOptions();
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

        if (buttonIndex == INPUT_INDEX_RIGHT || buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_B) {
            chooseMainMenuOption();
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_START) {
            saveHackOptions();
            if (gameHasStarted == 0) {
                menuDisplay_showMenu(MENU_LISTING_CHOOSE_GAME);
            } else {
                cartLoader_applyHackOptions(gameHasStarted);
                modConsole_applyHackOptions();
                modConsole_applyNetworkOptions();
                // menuDisplay_hideMenu();
                menuDisplay_showMenu(MENU_LISTING_IN_GAME);
            }
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
                cartLoader_loadSaveStateForQuitMenu();
                cartLoader_loadRandomRom();
                saveStateWasLoaded = 1;
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

    if (activeMenu == MENU_LISTING_RAM_DETECTIVE) {
        if (buttonIndex == INPUT_INDEX_UP) {
            ramDetectiveIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            ramDetectiveIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_LEFT) {
            ramDetectivePressDPadDir(-1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_RIGHT) {
            ramDetectivePressDPadDir(1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C) {
            ramDetectivePressFaceButton(1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B) {
            ramDetectivePressFaceButton(-1);
            refreshMenu();
            return 1;
        }


        if (buttonIndex == INPUT_INDEX_START) {
            menuDisplay_hideMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_RAM_DETECTIVE) {
        if (buttonIndex == INPUT_INDEX_UP) {
            ramDetectiveIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            ramDetectiveIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_LEFT) {
            ramDetectivePressDPadDir(-1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_RIGHT) {
            ramDetectivePressDPadDir(1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C) {
            ramDetectivePressFaceButton(1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B) {
            ramDetectivePressFaceButton(-1);
            refreshMenu();
            return 1;
        }


        if (buttonIndex == INPUT_INDEX_START) {
            menuDisplay_hideMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_PIXEL_DETECTIVE) {
        if (buttonIndex == INPUT_INDEX_UP) {
            pixelDetectiveIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            pixelDetectiveIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_LEFT) {
            pixelDetectivePressDPadDir(-1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_RIGHT) {
            pixelDetectivePressDPadDir(1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C) {
            pixelDetectivePressFaceButton(1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B) {
            pixelDetectivePressFaceButton(-1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_START) {
            menuDisplay_hideMenu();
            return 1;
        }
    }


    if (activeMenu == MENU_LISTING_GAME_SWAP_OPITONS) {
        if (buttonIndex == INPUT_INDEX_UP) {
            gameSwapOptionIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            gameSwapOptionIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementGameSwapOption(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementGameSwapOption(1);
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_QUALITY_OF_LIFE) {
        if (buttonIndex == INPUT_INDEX_UP) {
            qualityOfLifeOptionIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            qualityOfLifeOptionIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementQualityOfLifeOption(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementQualityOfLifeOption(1);
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_SAVE_STATE_OPTIONS) {
        if (buttonIndex == INPUT_INDEX_UP) {
            saveStateOptionIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            saveStateOptionIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementSaveStateOption(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementSaveStateOption(1);
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_SONIC_SPECIFIC_OPTIONS) {
        if (buttonIndex == INPUT_INDEX_UP) {
            sonicSpecificOptionIndex--;
            if (sonicSpecificOptionIndex == 3) {
                sonicSpecificOptionIndex --;
            }
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            sonicSpecificOptionIndex++;
            if (sonicSpecificOptionIndex == 3) {
                sonicSpecificOptionIndex ++;
            }
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementSonicSpecificOption(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementSonicSpecificOption(1);
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_VISUALS_OPTIONS) {
        if (buttonIndex == INPUT_INDEX_UP) {
            visualsOptionIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            visualsOptionIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementVisualsOption(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementVisualsOption(1);
            refreshMenu();
            return 1;
        }
    }
    
    if (activeMenu == MENU_LISTING_NETWORKING) {
        if (buttonIndex == INPUT_INDEX_UP) {
            networkingOptionsIndex--;
            while (networkingOptionsIndex == 2 || networkingOptionsIndex == 3 || networkingOptionsIndex == 4 || networkingOptionsIndex == 5 || networkingOptionsIndex == 10 || networkingOptionsIndex == 12) {
                networkingOptionsIndex --;
            }
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            networkingOptionsIndex++;
            while (networkingOptionsIndex == 2 || networkingOptionsIndex == 3 || networkingOptionsIndex == 4 || networkingOptionsIndex == 5 || networkingOptionsIndex == 10 || networkingOptionsIndex == 12) {
                networkingOptionsIndex ++;
            }
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementNetworkOption(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementNetworkOption(1);
            refreshMenu();
            return 1;
        }
    }

    return 0;
}

void incrementNetworkOption(int direction) {
    if (networkingOptionsIndex == 0) {
        networkOptions.networkingIsActive += direction;
    }
    if (networkingOptionsIndex == 1) {
        networkOptions.allowSoloEffectswhenNetworked += direction;
    }
    if (networkingOptionsIndex == 6) {
        networkOptions.sendSwitchGame += direction;
    }
    if (networkingOptionsIndex == 7) {
        networkOptions.sendSpeedUp += direction;
    }
    if (networkingOptionsIndex == 8) {
        networkOptions.sendRandomiseVelocity += direction;
    }
    if (networkingOptionsIndex == 9) {
        networkOptions.sendWriteIntoLevelDifficulty += direction;
    }

    if (networkingOptionsIndex == 11) {
        char action[0x100];
        sprintf(action, "");
        action[0] = NETWORK_MSG_REQUEST_RULES;
        cartLoader_writeActionToNetwork(action);

        networkOptions.awaitingOpponentSettingsState = 1;
    }


    if (networkingOptionsIndex == 13) {
        networkOptions.awaitingOpponentSettingsState = 0;
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
}

void incrementGameSwapOption(int direction) {
    if (gameSwapOptionIndex == 0) {
        hackOptions.switchGameType += direction;
    }
    if (gameSwapOptionIndex == 1) {
        hackOptions.cooldownOnSwitch += direction;
    }
    if (gameSwapOptionIndex == 2) {
        hackOptions.copyVram += direction;
    }
    if (gameSwapOptionIndex == 3) {
        hackOptions.swapOrder += direction;
    }
    if (gameSwapOptionIndex == 4) {
        hackOptions.shouldShowSwapCount += direction;
    }

    if (gameSwapOptionIndex == 5) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
}

void incrementQualityOfLifeOption(int direction) {
    if (qualityOfLifeOptionIndex == 0) {
        hackOptions.infiniteLives += direction;
    }
    if (qualityOfLifeOptionIndex == 1) {
        hackOptions.infiniteTime += direction;
    }
    if (qualityOfLifeOptionIndex == 2) {
        hackOptions.shouldWriteToLog += direction;
    }

    if (qualityOfLifeOptionIndex == 3) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
}

void incrementSaveStateOption(int direction) {
    if (saveStateOptionIndex == 0) {
        hackOptions.loadFromSavedState += direction;
    }
    if (saveStateOptionIndex == 1) {
        hackOptions.automaticallySaveStatesFreq += direction;
    }
    if (saveStateOptionIndex == 2) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }    
}

void incrementSonicSpecificOption(int direction) {
    if (sonicSpecificOptionIndex == 0) {
        hackOptions.speedUpOnRing += direction;
    }
    if (sonicSpecificOptionIndex == 1) {
        menuDisplay_showMenu(MENU_LISTING_PERSIST_VALUES);
    }
    if (sonicSpecificOptionIndex == 2) {
        hackOptions.overwriteLevelType += direction;
    }
    if (sonicSpecificOptionIndex == 4) {
        hackOptions.overwriteLevelDifficulty += direction;
    }
    if (sonicSpecificOptionIndex == 5) {
        hackOptions.randomiseVelocityOnRing += direction;
    }

    if (sonicSpecificOptionIndex == 6) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
}

void incrementVisualsOption(int direction) {
    if (visualsOptionIndex == 0) {
        hackOptions.shouldSortColours += direction;
    }
    if (visualsOptionIndex == 1) {
        hackOptions.limitedColourType += direction;
    }
    if (visualsOptionIndex == 2) {
        hackOptions.shouldHideLayers += direction;
    }

    if (visualsOptionIndex == 3) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
}

void ramDetectivePressFaceButton(int direction) {
    if (ramDetectiveIndex == 0) {
        ramDetectiveOptions.startLoc[ramDetectiveOptions.startValueIndex] += direction;
    }
    if (ramDetectiveIndex == 1) {
        ramDetectiveOptions.endLoc[ramDetectiveOptions.endValueIndex] += direction;
    }
    if (ramDetectiveIndex == 2) {
        ramDetectiveOptions.seekValue[ramDetectiveOptions.seekValueIndex] += direction;
    }
    if (ramDetectiveIndex == 3) {
        ramDetectiveOptions.minFrames += direction * 10;
    }
    if (ramDetectiveIndex == 4) {
        ramDetectiveOptions.shouldShow += direction;
    }

    for (int trackerIdx = 0; trackerIdx < 8; trackerIdx++) {
        if (ramDetectiveIndex == 5 + trackerIdx) {
            int col = ramDetectiveOptions.trackerValueIndexes[trackerIdx];
            ramDetectiveOptions.trackerLocations[trackerIdx][col] += direction;
        } 
    }

    if (ramDetectiveIndex == 13) {
        ramDetectiveOptions.shouldShowTracker += direction;
    }

    if (ramDetectiveIndex == 14) {
        modConsole_flagToLogRamState();
    }
    if (ramDetectiveIndex == 15) {
        clearLogRamState();
    }
}

void pixelDetectivePressFaceButton(int direction) {
    for (int trackerIdx = 0; trackerIdx < 8; trackerIdx++) {
        if (pixelDetectiveIndex == trackerIdx) {
            int col = pixelDetectiveOptions.coordsListingIndex;
            pixelDetectiveOptions.coordsListings[trackerIdx][col] += direction;
        } 
    }

    if (pixelDetectiveIndex == 8) {
        pixelDetectiveOptions.shouldShow += direction;
    }
}

void ramDetectivePressDPadDir(int direction) {
    if (ramDetectiveIndex == 0) {
        ramDetectiveOptions.startValueIndex += direction;
    }
    if (ramDetectiveIndex == 1) {
        ramDetectiveOptions.endValueIndex += direction;
    }
    if (ramDetectiveIndex == 2) {
        ramDetectiveOptions.seekValueIndex += direction;
    }
    if (ramDetectiveIndex == 3) {
        ramDetectiveOptions.minFrames += direction * 10;
    }
    if (ramDetectiveIndex == 4) {
        ramDetectiveOptions.shouldShow += direction;
    }

    for (int trackerIdx = 0; trackerIdx < 8; trackerIdx++) {
        if (ramDetectiveIndex == 5 + trackerIdx) {
            ramDetectiveOptions.trackerValueIndexes[trackerIdx] += direction;
        } 
    }

    if (ramDetectiveIndex == 13) {
        ramDetectiveOptions.shouldShowTracker += direction;
    }

    if (ramDetectiveIndex == 14) {
        modConsole_flagToLogRamState();
    }
    if (ramDetectiveIndex == 15) {
        clearLogRamState();
    }
}

void pixelDetectivePressDPadDir(int direction) {
    for (int trackerIdx = 0; trackerIdx < 8; trackerIdx++) {
        if (pixelDetectiveIndex == trackerIdx) {
            pixelDetectiveOptions.coordsListingIndex += direction;
        } 
    }

    if (pixelDetectiveIndex == 8) {
        pixelDetectiveOptions.shouldShow += direction;
    }
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
    if (index == 5) {
        persistValuesOptions.score = 1 - persistValuesOptions.score;
    }

    if (index == 6) {
        menuDisplay_showMenu(MENU_LISTING_SONIC_SPECIFIC_OPTIONS);
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
        cartLoader_loadSaveStateForQuitMenu();
        saveSaveStateForCurrentGame();
        cartLoader_saveAllSaveStatesToDisk();
    }
    if (inGameOptionIndex == 3) {
        cartLoader_loadSaveStateForQuitMenu();
        cartLoader_loadAllSaveStatesFromDisk();
        cartLoader_loadSaveStateForCurrentGame();
    }
    if (inGameOptionIndex == 4) {
        cartLoader_loadSaveStateForQuitMenu();
        vdp_setShouldRandomiseColours(0);
        cartLoader_removeCurrentGameFromRandomiser();
        saveStateWasLoaded = 1;
    }
    if (inGameOptionIndex == 5) {
        randomisedGameIndex = cartLoader_getActiveCartIndex();
        queuedMenu = MENU_LISTING_RANDOMISED_ROMS;
    }
    if (inGameOptionIndex == 6) {
        modConsole_queuePanic();
    }
    if (inGameOptionIndex == 7) {
        cartLoader_loadSaveStateForQuitMenu();
        modConsole_activateReset();
        saveStateWasLoaded = 1;
    }
    if (inGameOptionIndex == 8) {
        ramDetectiveIndex = 0;
        for (int i = 0; i < 0x10000; i++) {
            trackedRamFrameCounts[i] = 0;
        }
        queuedMenu = MENU_LISTING_RAM_DETECTIVE;
    }
    if (inGameOptionIndex == 9) {
        pixelDetectiveIndex = 0;
        queuedMenu = MENU_LISTING_PIXEL_DETECTIVE;
    }

    inGameOptionIndex = 0;
}

void chooseMainMenuOption() {
    if (optionsItemIndex == 0) {
        gameSwapOptionIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_GAME_SWAP_OPITONS);
    }

    if (optionsItemIndex == 1) {
        qualityOfLifeOptionIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_QUALITY_OF_LIFE);
    }

    if (optionsItemIndex == 2) {
        saveStateOptionIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_SAVE_STATE_OPTIONS);
    }

    if (optionsItemIndex == 3) {
        sonicSpecificOptionIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_SONIC_SPECIFIC_OPTIONS);
    }

    if (optionsItemIndex == 4) {
        visualsOptionIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_VISUALS_OPTIONS);
    }
    
    if (optionsItemIndex == 5) {
        networkingOptionsIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_NETWORKING);
    }

    if (optionsItemIndex == 6) {
        saveHackOptions();
        if (gameHasStarted == 0) {
            menuDisplay_showMenu(MENU_LISTING_CHOOSE_GAME);
        } else {
            cartLoader_applyHackOptions(gameHasStarted);
            modConsole_applyHackOptions();
            modConsole_applyNetworkOptions();
            // menuDisplay_hideMenu();
            menuDisplay_showMenu(MENU_LISTING_IN_GAME);
        }
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

    char romCountMsg[0x100];
    sprintf(romCountMsg, "--- found %d roms ---", cartLoader_getRomCount());
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 56, romCountMsg, 5);

    int startYPos = 72;
    if (cartLoader_getFoundZipCount() > 0) {
        char zipMsg[0x100];
        sprintf(zipMsg, "Found %d zip files", cartLoader_getFoundZipCount());
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos, zipMsg, 5);
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos + 8, "You must unzip them before", 5);
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos + 16, "you can play them", 5);
        startYPos += 32;
    }

    if (cartLoader_getRomCount() == 0) {
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos + 8, "Please put files of type", 5);
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos + 16, ".md .smd .sms .bin .gen", 5);
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos + 24, "in your _magicbox folder", 5);
        startYPos += 32;
    }

    int didBreak = 0;
    for (int i = 0; i < cartLoader_getRomCount(); i++)
    {
        char fileNameBuf[0x100];
        cartLoader_getRomFileName(i, fileNameBuf);
        char shortenedBuf[0x100];
        writeShortenedFileName(fileNameBuf, shortenedBuf, maxFileNameLength);
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos, shortenedBuf, 5);
        startYPos += 8;
        if (startYPos >= DEFAULT_HEIGHT - 32) {
            didBreak = 1;
            break;
        }
    }  
    if (didBreak == 1) {
        layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, startYPos, "... and more", 5);
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
    for (int i = startIndex; i <= endIndex; i++) {
        char fileNameBuf[0x100];
        cartLoader_getRomFileName(i, fileNameBuf);
        char shortenedBuf[0x100];
        writeShortenedFileName(fileNameBuf, shortenedBuf, maxFileNameLength);

        if (i == chosenGameIndex) {
            char newNameBuf[0x100];
            sprintf(newNameBuf, ">>  %s", shortenedBuf);
            layerRenderer_writeWord256WithBorder(0, 16, yPos, newNameBuf, 5, 1, 0);
        } else {
            char newNameBuf[0x100];
            for (int j = 0; j < 0xF0; j++) {
                newNameBuf[j + 3] = shortenedBuf[j];
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

    int lineCount = 7;
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

    if (menuDisplay_shouldGameSwapOptionsShowAsOn() != 0) {
        sprintf(lines[0], "[ON] Game swapping >");
    } else {
        if (networkOptions.allowSoloEffectswhenNetworked != 0) {
            sprintf(lines[0], "[BLOCKED] Game swapping >");
        } else {
            sprintf(lines[0], "     Game swapping >");
        }
    }
    
    if (menuDisplay_shouldQualityOfLifeOptionsShowAsOn() != 0) {
        sprintf(lines[1], "[ON] Quality of life >");
    } else {
        sprintf(lines[1], "     Quality of life >");
    }

    if (menuDisplay_shouldSaveStateOptionsShowAsOn() != 0) {
        sprintf(lines[2], "[ON] Save states >");
    } else {
        sprintf(lines[2], "     Save states >");
    }

    if (menuDisplay_shouldSonicSpecificOptionsShowAsOn() != 0) {
        sprintf(lines[3], "[ON] Sonic-specific >");
    } else {
        sprintf(lines[3], "     Sonic-specific >");
    }

    if (menuDisplay_shouldVisualsOptionsShowAsOn() != 0) {
        sprintf(lines[4], "[ON] Visuals >");
    } else {
        sprintf(lines[4], "     Visuals >");
    }
    
    if (menuDisplay_shouldNetworkingOptionsShowAsOn() != 0) {
        sprintf(lines[5], "[ON] Networking >");
    } else {
        sprintf(lines[5], "     Networking >");
    }

    sprintf(lines[6], "Start game");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "Start game") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == optionsItemIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "   %s", lines[i]);
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

    int lineCount = 11;
    char lines[lineCount][0x80];

    int hintLineCount = 5;
    char hintLines[hintLineCount][0x80];
    for (int i = 0; i < hintLineCount; i++) {
        sprintf(hintLines[i], "");
    }

    if (inGameOptionIndex < 0) {
        inGameOptionIndex = 0;
    }
    if (inGameOptionIndex >= lineCount) {
        inGameOptionIndex = lineCount - 1;
    }

    sprintf(lines[0], "Back to game");
    sprintf(lines[1], "Change hack options >>");
    sprintf(lines[2], "Save all game states to disk");
    sprintf(lines[3], "Load all game states from disk" );
    sprintf(lines[4], "Remove this game from randomiser" );
    sprintf(lines[5], "Toggle games in randomiser >>" );
    sprintf(lines[6], "Kill Sonic");
    if (inGameOptionIndex == 6) {
        sprintf(hintLines[0], "* You can also press DOWN + B + START");
        sprintf(hintLines[1], "  In-game to kill sonic");
        sprintf(hintLines[2], "* Only works in Sonic 1, 2, 3 and");
        sprintf(hintLines[3], "  Sonic & Knuckles");
    }

    sprintf(lines[7], "Reset Game");
    sprintf(lines[8], "Ram detective tool >>");
    if (inGameOptionIndex == 8) {
        sprintf(hintLines[0], "* Use this to figure out what ram");
        sprintf(hintLines[1], "  values can be used to get specific");
        sprintf(hintLines[2], "  game events");
    }
    sprintf(lines[9], "Pixel detective tool >>");
    sprintf(lines[10], "Back to game");

    int yPos = 48;
    for (int i = 0; i < lineCount; i++) {
        char lineBuf[0x100];
        int colour = 5;
        if (i == inGameOptionIndex) {
            sprintf(lineBuf, ">>   %s", lines[i]);
            colour = 6;
        } else {
            sprintf(lineBuf, "   %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, lineBuf, colour, 1, 0);
        yPos += 8;
    }

    yPos += 8;

    for (int i = 0; i < hintLineCount; i++) {
        layerRenderer_writeWord256WithBorder(0, 16, yPos, hintLines[i], 6, 1, 0);
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
        char shortenedBuf[0x100];
        writeShortenedFileName(fileNameBuf, shortenedBuf, maxFileNameLength);

        if (cartLoader_gameIsBlockedFromRandomiser(i)) {
            sprintf(gameRandomStates[i], "off: %s", shortenedBuf);
        } else {
            sprintf(gameRandomStates[i], "on:  %s", shortenedBuf);
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

    int lineCount = 7;
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

    if (persistValuesOptions.score == 0) {
        sprintf(lines[5], "score:        no");
    } else {
        sprintf(lines[5], "score:       yes");
    }

    sprintf(lines[6], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }


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


void showRamDetectiveMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Ram Detective", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to confirm ---", 5);

    int lineCount = 16;
    char lines[lineCount][0x80];

    if (ramDetectiveIndex < 0) {
        ramDetectiveIndex = lineCount - 1;
    }
    if (ramDetectiveIndex >= lineCount) {
        ramDetectiveIndex = 0;
    }

    if (ramDetectiveOptions.startValueIndex < 0) {
        ramDetectiveOptions.startValueIndex = 0;
    }
    if (ramDetectiveOptions.startValueIndex > 3) {
        ramDetectiveOptions.startValueIndex = 3;
    }

    if (ramDetectiveOptions.endValueIndex < 0) {
        ramDetectiveOptions.endValueIndex = 0;
    }
    if (ramDetectiveOptions.endValueIndex > 3) {
        ramDetectiveOptions.endValueIndex = 3;
    }

    if (ramDetectiveOptions.seekValueIndex < 0) {
        ramDetectiveOptions.seekValueIndex = 0;
    }
    if (ramDetectiveOptions.seekValueIndex > 1) {
        ramDetectiveOptions.seekValueIndex = 1;
    }

    if (ramDetectiveOptions.shouldShow < 0) {
        ramDetectiveOptions.shouldShow = 1;
    }
    if (ramDetectiveOptions.shouldShow > 1) {
        ramDetectiveOptions.shouldShow = 0;
    }

    if (ramDetectiveOptions.minFrames < 0) {
        ramDetectiveOptions.minFrames = 300;
    }
    if (ramDetectiveOptions.minFrames > 300) {
        ramDetectiveOptions.minFrames = 0;
    }

    for (int trackIdx = 0; trackIdx < 8; trackIdx++) {
        if (ramDetectiveOptions.trackerValueIndexes[trackIdx] < 0) {
            ramDetectiveOptions.trackerValueIndexes[trackIdx] = 0;
        }
        if (ramDetectiveOptions.trackerValueIndexes[trackIdx] > 3) {
            ramDetectiveOptions.trackerValueIndexes[trackIdx] = 3;
        }
    }

    if (ramDetectiveOptions.shouldShowTracker < 0) {
        ramDetectiveOptions.shouldShowTracker = 1;
    }
    if (ramDetectiveOptions.shouldShowTracker > 1) {
        ramDetectiveOptions.shouldShowTracker = 0;
    }

    char startValuesText[4][0x10];
    for (int i = 0; i < 4; i++) {
        if (ramDetectiveOptions.startLoc[i] < 0) {
            ramDetectiveOptions.startLoc[i] = 0xF;
        }
        if (ramDetectiveOptions.startLoc[i] > 0xF) {
            ramDetectiveOptions.startLoc[i] = 0;
        }

        if (ramDetectiveIndex == 0 && ramDetectiveOptions.startValueIndex == i) {
            sprintf(startValuesText[i], "<%X>", ramDetectiveOptions.startLoc[i]);
        } else {
            sprintf(startValuesText[i], " %X ", ramDetectiveOptions.startLoc[i]);
        }
    }
    sprintf(lines[0], "START: %s%s%s%s", startValuesText[0], startValuesText[1], startValuesText[2], startValuesText[3]);

    char endValuesText[4][0x10];
    for (int i = 0; i < 4; i++) {
        if (ramDetectiveOptions.endLoc[i] < 0) {
            ramDetectiveOptions.endLoc[i] = 0xF;
        }
        if (ramDetectiveOptions.endLoc[i] > 0xF) {
            ramDetectiveOptions.endLoc[i] = 0;
        }

        if (ramDetectiveIndex == 1 && ramDetectiveOptions.endValueIndex == i) {
            sprintf(endValuesText[i], "<%X>", ramDetectiveOptions.endLoc[i]);
        } else {
            sprintf(endValuesText[i], " %X ", ramDetectiveOptions.endLoc[i]);
        }
    }
    sprintf(lines[1], "END:   %s%s%s%s", endValuesText[0], endValuesText[1], endValuesText[2], endValuesText[3]);

    char seekValuesText[2][0x10];
    for (int i = 0; i < 2; i++) {
        if (ramDetectiveOptions.seekValue[i] < 0) {
            ramDetectiveOptions.seekValue[i] = 0xF;
        }
        if (ramDetectiveOptions.seekValue[i] > 0xF) {
            ramDetectiveOptions.seekValue[i] = 0;
        }

        if (ramDetectiveIndex == 2 && ramDetectiveOptions.seekValueIndex == i) {
            sprintf(seekValuesText[i], "<%X>", ramDetectiveOptions.seekValue[i]);
        } else {
            sprintf(seekValuesText[i], " %X ", ramDetectiveOptions.seekValue[i]);
        }
    }
    sprintf(lines[2], "SEEK:  %s%s", seekValuesText[0], seekValuesText[1]);

    if (ramDetectiveOptions.minFrames < 0) {
        ramDetectiveOptions.minFrames = 0;
    }
    sprintf(lines[3], "MIN FRAMES: %d", ramDetectiveOptions.minFrames);

    if (ramDetectiveOptions.shouldShow == 0) {
        sprintf(lines[4], "SHOW SEEKER:   OFF");
    } else {
        sprintf(lines[4], "SHOW SEEKER:    ON");
    }

    for (int trackIdx = 0; trackIdx < 8; trackIdx++) {
        char trackValuesText[4][0x10];
        int lineIdx = 5 + trackIdx;
        for (int i = 0; i < 4; i++) {
            if (ramDetectiveOptions.trackerLocations[trackIdx][i] < 0) {
                ramDetectiveOptions.trackerLocations[trackIdx][i] = 0xF;
            }
            if (ramDetectiveOptions.trackerLocations[trackIdx][i] > 0xF) {
                ramDetectiveOptions.trackerLocations[trackIdx][i] = 0;
            }

            if (ramDetectiveIndex == lineIdx && ramDetectiveOptions.trackerValueIndexes[trackIdx] == i) {
                sprintf(trackValuesText[i], "<%X>", ramDetectiveOptions.trackerLocations[trackIdx][i]);
            } else {
                sprintf(trackValuesText[i], " %X ", ramDetectiveOptions.trackerLocations[trackIdx][i]);
            }
        }
        sprintf(lines[lineIdx], "Track %i:  %s%s%s%s", trackIdx, trackValuesText[0], trackValuesText[1], trackValuesText[2], trackValuesText[3]);
    }

    if (ramDetectiveOptions.shouldShowTracker == 0) {
        sprintf(lines[13], "SHOW TRACKER:   OFF");
    } else {
        sprintf(lines[13], "SHOW TRACKER:    ON");
    }

    sprintf(lines[14], "print seek to log now");
    sprintf(lines[15], "reset seek log counter");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        char toPrint[0x100];
        if (i == ramDetectiveIndex) {
            sprintf(toPrint, "> %s", lines[i]);
        } else {
            sprintf(toPrint, "  %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);
        yPos += 8;
    }
}

void showPixelDetectiveMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Pixel Detective", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 16, "--- press start to confirm ---", 5);

    int lineCount = 9;
    char lines[lineCount][0x80];

    if (pixelDetectiveIndex < 0) {
        pixelDetectiveIndex = lineCount - 1;
    }
    if (pixelDetectiveIndex >= lineCount) {
        pixelDetectiveIndex = 0;
    }

    if (pixelDetectiveOptions.coordsListingIndex < 0) {
        pixelDetectiveOptions.coordsListingIndex = 0;
    }
    if (pixelDetectiveOptions.coordsListingIndex > 3) {
        pixelDetectiveOptions.coordsListingIndex = 3;
    }

    for (int trackIdx = 0; trackIdx < 8; trackIdx++) {
        char trackValuesText[4][0x10];
        int lineIdx = trackIdx;
        for (int i = 0; i < 4; i++) {
            if (pixelDetectiveOptions.coordsListings[trackIdx][i] < 0) {
                pixelDetectiveOptions.coordsListings[trackIdx][i] = 0xF;
            }
            if (pixelDetectiveOptions.coordsListings[trackIdx][i] > 0xF) {
                pixelDetectiveOptions.coordsListings[trackIdx][i] = 0;
            }

            if (pixelDetectiveIndex == lineIdx && pixelDetectiveOptions.coordsListingIndex == i) {
                sprintf(trackValuesText[i], "<%X>", pixelDetectiveOptions.coordsListings[trackIdx][i]);
            } else {
                sprintf(trackValuesText[i], " %X ", pixelDetectiveOptions.coordsListings[trackIdx][i]);
            }
        }
        sprintf(lines[lineIdx], "Track %i:  X:%s%s, Y:%s%s", trackIdx, trackValuesText[0], trackValuesText[1], trackValuesText[2], trackValuesText[3]);
    }

    if (pixelDetectiveOptions.shouldShow > 1) {
        pixelDetectiveOptions.shouldShow = 0;
    }
    if (pixelDetectiveOptions.shouldShow < 0) {
        pixelDetectiveOptions.shouldShow = 1;
    }

    if (pixelDetectiveOptions.shouldShow == 0) {
        sprintf(lines[8], "SHOW PIXEL VALUES:   OFF");
    } else {
        sprintf(lines[8], "SHOW PIXEL VALUES:    ON");
    }

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        char toPrint[0x100];
        if (i == pixelDetectiveIndex) {
            sprintf(toPrint, "> %s", lines[i]);
        } else {
            sprintf(toPrint, "  %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);
        yPos += 8;
    }
}


void showGameSwapOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Game swapping", 5);

    int lineCount = 6;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
    }

    if (gameSwapOptionIndex < 0) {
        gameSwapOptionIndex = lineCount - 1;
    }
    if (gameSwapOptionIndex >= lineCount) {
        gameSwapOptionIndex = 0;
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
        blockedLines[3] = 1;
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

    if (hackOptions.swapOrder > 1) {
        hackOptions.swapOrder = 0;
    }
    if (hackOptions.swapOrder < 0) {
        hackOptions.swapOrder = 3;
    }
    if (hackOptions.swapOrder == 0) {
        sprintf(lines[3], "Swap order:           random");
    } else {
        sprintf(lines[3], "Swap order:     alphabetical");
    }
    
    if (hackOptions.shouldShowSwapCount > 1) {
        hackOptions.shouldShowSwapCount = 0;
    }
    if (hackOptions.shouldShowSwapCount < 0) {
        hackOptions.shouldShowSwapCount = 1;
    }
    if (hackOptions.shouldShowSwapCount == 0) {
        sprintf(lines[4], "Show swap counter:       OFF");
    } else {
        sprintf(lines[4], "Show swap counter:        ON");
    }

    sprintf(lines[5], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == gameSwapOptionIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "   %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);

        if (blockedLines[i] != 0) {
            layerRenderer_fill(0, 16 + 32, yPos + 3, DEFAULT_WIDTH - 48 - 16, 2, 5);
        }

        yPos += 8;
    }
}

void showQualityOfLifeOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Quality of life", 5);

    int lineCount = 4;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
    }

    if (qualityOfLifeOptionIndex < 0) {
        qualityOfLifeOptionIndex = lineCount - 1;
    }
    if (qualityOfLifeOptionIndex >= lineCount) {
        qualityOfLifeOptionIndex = 0;
    }

    if (hackOptions.infiniteLives > 1) {
        hackOptions.infiniteLives = 0;
    }
    if (hackOptions.infiniteLives < 0) {
        hackOptions.infiniteLives = 1;
    }
    if (hackOptions.infiniteLives == 1) {
        sprintf(lines[0], "Infinite lives:           ON");
    } else {
        sprintf(lines[0], "Infinite lives:          OFF");
    }

    if (hackOptions.infiniteTime > 1) {
        hackOptions.infiniteTime = 0;
    }
    if (hackOptions.infiniteTime < 0) {
        hackOptions.infiniteTime = 1;
    }
    if (hackOptions.infiniteTime == 1) {
        sprintf(lines[1], "Infinite time:            ON");
    } else {
        sprintf(lines[1], "Infinite time:           OFF");
    }

    if (hackOptions.shouldWriteToLog > 1) {
        hackOptions.shouldWriteToLog = 0;
    }
    if (hackOptions.shouldWriteToLog < 0) {
        hackOptions.shouldWriteToLog = 1;
    }
    if (hackOptions.shouldWriteToLog == 0) {
        sprintf(lines[2], "Write to debug log:      OFF");
    } else {
        sprintf(lines[2], "Write to debug log:       ON"); 
    }

    sprintf(lines[3], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == qualityOfLifeOptionIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "   %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);

        if (blockedLines[i] != 0) {
            layerRenderer_fill(0, 16 + 32, yPos + 3, DEFAULT_WIDTH - 48 - 16, 2, 5);
        }

        yPos += 8;
    }
}

void showSaveStateOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Save states", 5);

    int lineCount = 3;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
    }

    if (saveStateOptionIndex < 0) {
        saveStateOptionIndex = lineCount - 1;
    }
    if (saveStateOptionIndex >= lineCount) {
        saveStateOptionIndex = 0;
    }
    
    if (hackOptions.loadFromSavedState > 1) {
        hackOptions.loadFromSavedState = 0;
    }
    if (hackOptions.loadFromSavedState < 0) {
        hackOptions.loadFromSavedState = 1;
    }
    if (hackOptions.loadFromSavedState == 0) {
        sprintf(lines[0], "Begin with saved state:  OFF");
    } else {
        sprintf(lines[0], "Begin with saved state:   ON");
    }

    if (hackOptions.automaticallySaveStatesFreq > 5) {
        hackOptions.automaticallySaveStatesFreq = 0;
    }
    if (hackOptions.automaticallySaveStatesFreq < 0) {
        hackOptions.automaticallySaveStatesFreq = 5;
    }
    if (hackOptions.automaticallySaveStatesFreq == 0) {
        sprintf(lines[1], "Auto-save state:         OFF");
    } else if (hackOptions.automaticallySaveStatesFreq == 1) {
        sprintf(lines[1], "Auto-save state: EVERY 1 min");
    } else if (hackOptions.automaticallySaveStatesFreq == 2) {
        sprintf(lines[1], "Auto-save state: EVERY 5 mins");
    } else if (hackOptions.automaticallySaveStatesFreq == 3) {
        sprintf(lines[1], "Auto-save state: EVERY 10 min");
    } else if (hackOptions.automaticallySaveStatesFreq == 4) {
        sprintf(lines[1], "Auto-save state: EVERY 15 min");
    } else if (hackOptions.automaticallySaveStatesFreq == 5) {
        sprintf(lines[1], "Auto-save state: EVERY 5 secs");
    }

    sprintf(lines[2], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == saveStateOptionIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "   %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);

        if (blockedLines[i] != 0) {
            layerRenderer_fill(0, 16 + 32, yPos + 3, DEFAULT_WIDTH - 48 - 16, 2, 5);
        }

        yPos += 8;
    }
}

void showSonicSpecificOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Sonic-specific options", 5);

    int lineCount = 7;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }

    if (sonicSpecificOptionIndex < 0) {
        sonicSpecificOptionIndex = lineCount - 1;
    }
    if (sonicSpecificOptionIndex >= lineCount) {
        sonicSpecificOptionIndex = 0;
    }
    
    if (hackOptions.speedUpOnRing > 1) {
        hackOptions.speedUpOnRing = 0;
    }
    if (hackOptions.speedUpOnRing < 0) {
        hackOptions.speedUpOnRing = 1;
    }
    if (hackOptions.speedUpOnRing == 1) {
        sprintf(lines[0], "Speed up on ring:         ON");
    } else {
        sprintf(lines[0], "Speed up on ring:        OFF");
    }
    linesWithBreakAfter[0] = 1;

    if (menuDisplay_shouldPersistValueOptionsShowAsOn() != 0) {
        sprintf(lines[1], "[ON] Persist values between games >>");
    } else {
        sprintf(lines[1], "Persist values between games >>");
    }
    linesWithBreakAfter[1] = 1;

    if (hackOptions.overwriteLevelType > 2) {
        hackOptions.overwriteLevelType = 0;
    }
    if (hackOptions.overwriteLevelType < 0) {
        hackOptions.overwriteLevelType = 2;
    }
    sprintf(lines[2], "Write into level data on");
    if (hackOptions.overwriteLevelType == 0) {
        sprintf(lines[3], "             get ring:     off");
        blockedLines[3] = 0;
    } else if (hackOptions.overwriteLevelType == 1) {
        sprintf(lines[3], "    get ring:   Random numbers");
    } else {
        sprintf(lines[3], "             get ring:  zeroes");
    }
    linesWithBreakAfter[3] = 1;

    if (hackOptions.overwriteLevelDifficulty > 2) {
        hackOptions.overwriteLevelDifficulty = 0;
    }
    if (hackOptions.overwriteLevelDifficulty < 0) {
        hackOptions.overwriteLevelDifficulty = 2;
    }
    if (hackOptions.overwriteLevelDifficulty == 0) {
        sprintf(lines[4], "level write difficulty:   easy");
    } else if (hackOptions.overwriteLevelDifficulty == 1) {
        sprintf(lines[4], "level write difficulty: medium");
    } else {
        sprintf(lines[4], "level write difficulty:   hard");
    }
    linesWithBreakAfter[4] = 1;

    if (hackOptions.randomiseVelocityOnRing > 1) {
        hackOptions.randomiseVelocityOnRing = 0;
    }
    if (hackOptions.randomiseVelocityOnRing < 0) {
        hackOptions.randomiseVelocityOnRing = 1;
    }
    if (hackOptions.randomiseVelocityOnRing == 1) {
        sprintf(lines[5], "Random velocity on ring:    ON");
    } else {
        sprintf(lines[5], "Random velocity on ring:   OFF");
    }
    linesWithBreakAfter[5] = 1;

    sprintf(lines[6], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == sonicSpecificOptionIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "   %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);

        if (blockedLines[i] != 0) {
            layerRenderer_fill(0, 16 + 32, yPos + 3, DEFAULT_WIDTH - 48 - 16, 2, 5);
        }

        yPos += 8;
        if (linesWithBreakAfter[i] != 0) {
            yPos += 8;
        }
    }
}

void showVisualsOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Visuals", 5);

    int lineCount = 4;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
    }

    if (visualsOptionIndex < 0) {
        visualsOptionIndex = lineCount - 1;
    }
    if (visualsOptionIndex >= lineCount) {
        visualsOptionIndex = 0;
    }
    
    if (hackOptions.shouldSortColours > 1) {
        hackOptions.shouldSortColours = 0;
    }
    if (hackOptions.shouldSortColours < 0) {
        hackOptions.shouldSortColours = 1;
    }
    if (hackOptions.shouldSortColours == 0) {
        sprintf(lines[0], "Sort pixels by colour:    OFF");
    } else {
        sprintf(lines[0], "Sort pixels by colour:     ON");
    }

    if (hackOptions.limitedColourType > 5) {
        hackOptions.limitedColourType = 0;
    }
    if (hackOptions.limitedColourType < 0) {
        hackOptions.limitedColourType = 5;
    }
    if (hackOptions.limitedColourType == 0) {
        sprintf(lines[1], "Limit palettes:           OFF");
    } else if (hackOptions.limitedColourType == 1) {
        sprintf(lines[1], "Limit palettes:     2 COLOURS");
    } else if (hackOptions.limitedColourType == 2) {
        sprintf(lines[1], "Limit palettes:     3 COLOURS");
    } else if (hackOptions.limitedColourType == 3) {
        sprintf(lines[1], "Limit palettes:     4 COLOURS");
    } else if (hackOptions.limitedColourType == 4) {
        sprintf(lines[1], "Limit palettes:     5 COLOURS");
    } else if (hackOptions.limitedColourType == 5) {
        sprintf(lines[1], "Limit palettes:    10 COLOURS");
    }

    if (hackOptions.shouldHideLayers > 2) {
        hackOptions.shouldHideLayers = 0;
    }
    if (hackOptions.shouldHideLayers < 0) {
        hackOptions.shouldHideLayers = 2;
    }
    if (hackOptions.shouldHideLayers == 0) {
        sprintf(lines[2], "Hide layers:              OFF");
    } else if (hackOptions.shouldHideLayers == 1) {
        sprintf(lines[2], "Hide layers:        NO SPRITES");
    } else if (hackOptions.shouldHideLayers == 2) {
        sprintf(lines[2], "Hide layers:    NO BACKGROUNDS");
    }
    sprintf(lines[3], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == visualsOptionIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "   %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);

        if (blockedLines[i] != 0) {
            layerRenderer_fill(0, 16 + 32, yPos + 3, DEFAULT_WIDTH - 48 - 16, 2, 5);
        }

        yPos += 8;
    }
}

void showNetworkingOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Networking", 5);

    int lineCount = 14;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
    }

    if (networkingOptionsIndex < 0) {
        networkingOptionsIndex = lineCount - 1;
    }
    if (networkingOptionsIndex >= lineCount) {
        networkingOptionsIndex = 0;
    }
    
    if (networkOptions.networkingIsActive > 1) {
        networkOptions.networkingIsActive = 0;
    }
    if (networkOptions.networkingIsActive < 0) {
        networkOptions.networkingIsActive = 1;
    }
    if (networkOptions.networkingIsActive == 0) {
        sprintf(lines[0], "Networked play:           OFF");
    } else {
        sprintf(lines[0], "Networked play:           ON");
    }

    sprintf(lines[1], "Enable single-player effects");
    if (networkOptions.allowSoloEffectswhenNetworked > 1) {
        networkOptions.allowSoloEffectswhenNetworked = 0;
    }
    if (networkOptions.allowSoloEffectswhenNetworked < 0) {
        networkOptions.allowSoloEffectswhenNetworked = 1;
    }
    if (networkOptions.allowSoloEffectswhenNetworked == 0) {
        sprintf(lines[2], "    during networked play:  NO");
    } else {
        sprintf(lines[2], "    during networked play: YES");
    }


    if (networkOptions.networkingIsActive == 0) {
        blockedLines[6] = 1;
        blockedLines[7] = 1;
        blockedLines[8] = 1;
        blockedLines[9] = 1;
    }

    sprintf(lines[3], "");
    sprintf(lines[4], "When you get a ring");
    sprintf(lines[5], "it will...");

    if (networkOptions.sendSwitchGame > 1) {
        networkOptions.sendSwitchGame = 0;
    }
    if (networkOptions.sendSwitchGame < 0) {
        networkOptions.sendSwitchGame = 1;
    }
    if (networkOptions.sendSwitchGame == 0) {
        sprintf(lines[6], "  Switch opponent's game:  OFF");
    } else {
        sprintf(lines[6], "  Switch opponent's game:   ON");
    }

    if (networkOptions.sendSpeedUp > 1) {
        networkOptions.sendSpeedUp = 0;
    }
    if (networkOptions.sendSpeedUp < 0) {
        networkOptions.sendSpeedUp = 1;
    }
    if (networkOptions.sendSpeedUp == 0) {
        sprintf(lines[7], "  Speed up opponent:       OFF");
    } else {
        sprintf(lines[7], "  Speed up opponent:        ON");
    }

    if (networkOptions.sendRandomiseVelocity > 1) {
        networkOptions.sendRandomiseVelocity = 0;
    }
    if (networkOptions.sendRandomiseVelocity < 0) {
        networkOptions.sendRandomiseVelocity = 1;
    }
    if (networkOptions.sendRandomiseVelocity == 0) {
        sprintf(lines[8], "  Randomise oppt velocity: OFF");
    } else {
        sprintf(lines[8], "  Randomise oppt velocity:  ON");
    }

    if (networkOptions.sendWriteIntoLevelDifficulty > 3) {
        networkOptions.sendWriteIntoLevelDifficulty = 0;
    }
    if (networkOptions.sendWriteIntoLevelDifficulty < 0) {
        networkOptions.sendWriteIntoLevelDifficulty = 3;
    }
    if (networkOptions.sendWriteIntoLevelDifficulty == 0) {
        sprintf(lines[9], "  Scramble oppt level:     OFF");
    } else if (networkOptions.sendWriteIntoLevelDifficulty == 1) {
        sprintf(lines[9], "  Scramble oppt level:   A BIT");
    } else if (networkOptions.sendWriteIntoLevelDifficulty == 2) {
        sprintf(lines[9], "  Scramble oppt level:   A LOT");
    } else if (networkOptions.sendWriteIntoLevelDifficulty == 3) {
        sprintf(lines[9], "  Scramble oppt level:   LOADS");
    }

    sprintf(lines[10], "");
    if (networkOptions.awaitingOpponentSettingsState == 0) {
        sprintf(lines[11], "Get opponent's settings");
    } else if (networkOptions.awaitingOpponentSettingsState == 1) {
        sprintf(lines[11], "Get opponent's settings (getting)");
    } else if (networkOptions.awaitingOpponentSettingsState == 2) {
        sprintf(lines[11], "Get opponent's settings (done!)");
    } 

    sprintf(lines[12], "");
    sprintf(lines[13], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == networkingOptionsIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "   %s", lines[i]);
        }

        layerRenderer_writeWord256WithBorder(0, 16, yPos, toPrint, 5, 1, 0);

        if (blockedLines[i] != 0) {
            layerRenderer_fill(0, 16 + 32, yPos + 3, DEFAULT_WIDTH - 48 - 16, 2, 5);
        }

        yPos += 8;
    }
}
