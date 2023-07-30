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
#include "gamepad.h"

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
static int ramEditingOptionsIndex = 0;
static int ramEditingLocationIndex = 0;
static int terminalLocationIndex = 0;
static int bossRushItemIndex = 0;
static int gameSuiteSelectIndex = 0;
static int ninesChallengeItemIndex = 0;
static int bossRushTriggerSelectItemIndex = 0;

static int majorVersion = 0;
static int minorVersion = 40;

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
static SecondaryHackOptions secondaryHackOptions;
static BossRushOptions bossRushOptions;
static NinesChallengeOptions ninesChallengeOptions;
static int logRamStateCounter[0x10000];

static int trackedRamFrameCounts[0x10000];
// static int trackedRamLocationCount = 0;

static int maxFileNameLength = 28;

static int trackedPixelValues[8];

static int activeTerminalRuleId = -1;

static int shouldRerollBossRushRandomTime = 0;
static int shouldRerollNinesChallengeRandomTime = 0;

static int terminalActiveRules = 0;

static int allowedGamesThisTerminal[16];
static int spacesUnderGamesThisTerminal[16];
static int gameCountThisTerminal = 0;
static int hasMappedRomsToLevels = 0;

static char currentRulesName[100];

static int requestedNinesChallengeStartRom = 0;

void menuDisplay_generateRulesNameForCurrentGame() {
    sprintf(currentRulesName, "");

    if (terminalActiveRules == TERMINAL_RULSET_SHUFFLER) {
        sprintf(currentRulesName, "Switch game whenever %s", cartLoader_getNameOfTriggerForActiveGame());
    }

    if (terminalActiveRules == TERMINAL_RULSET_SHUFFLER_WITH_VRAM) {
        sprintf(currentRulesName, "Switch game whenever %s and keep visual memory from the previous game", cartLoader_getNameOfTriggerForActiveGame());
    }

    if (terminalActiveRules == TERMINAL_RULSET_RINGS_MAKE_FASTER) {
        sprintf(currentRulesName, "Make sonic faster whenever he gets a ring", cartLoader_getNameOfTriggerForActiveGame());
    }

    if (terminalActiveRules == TERMINAL_RULSET_RINGS_CORRUPT_LEVEL) {
        sprintf(currentRulesName, "Write random numbers to level data whenever %s", cartLoader_getNameOfTriggerForActiveGame());
    }

    if (terminalActiveRules == TERMINAL_RULSET_RINGS_CORRUPT_RAM) {
        sprintf(currentRulesName, "Write random numbers to ram whenever %s", cartLoader_getNameOfTriggerForActiveGame());
    }
    
    if (terminalActiveRules == TERMINAL_RULSET_REMOVE_COLOUR) {
        sprintf(currentRulesName, "Remove colours from the universe whenever %s", cartLoader_getNameOfTriggerForActiveGame());
    }

    if (terminalActiveRules == TERMINAL_RULSET_NO_SPRITES_ALT) {
        sprintf(currentRulesName, "Sprites are invisible");
    }
    
    if (terminalActiveRules == TERMINAL_RULSET_NO_BACKGROUNDS_ALT) {
        sprintf(currentRulesName, "Only sprites are visible");
    }
        
    if (terminalActiveRules == TERMINAL_RULSET_SORT_COLOURS) {
        sprintf(currentRulesName, "Pixels are sorted by colour");
    }

    if (terminalActiveRules == TERMINAL_RULSET_BOSS_RUSH) {
        sprintf(currentRulesName, "Boss rush mode!");
    }
    
    if (terminalActiveRules == TERMINAL_RULSET_CONTROLLER) {
        sprintf(currentRulesName, "Controls change every 30 seconds       For 4 players with the big buttons!");
    }
}

int terminalRulesAreActive() {
    if (terminalActiveRules > 0) {
        return 1;
    }
    return 0;
}

char* menuDisplay_getCurrentRulesName() {
    return currentRulesName;
}

HackOptions menuDisplay_getHackOptions() {
    return hackOptions;
}

void menuDisplay_beginIdleMode() {
    terminalActiveRules = 0;
    menuDisplay_generateRulesNameForCurrentGame();

    menuDisplay_applyPresetRules(0);
    hackOptions.switchGameType = 2;
    hackOptions.copyVram = 1;
    setShouldUseControlsShuffle(0);
}

void menuDisplay_toggleVisibleLayers() {
    hackOptions.shouldHideLayers += 1 + (rand() % 2);
    hackOptions.shouldHideLayers = hackOptions.shouldHideLayers % 3;
}

void menuDisplay_showAllVisibleLayers() {
    hackOptions.shouldHideLayers = 0;
}

void menudisplay_applyToggleVRAMState(int vramState) {
    hackOptions.copyVram = vramState;
}


SecondaryHackOptions menuDisplay_getSecondaryHackOptions() {
    return secondaryHackOptions;
}

PersistValuesOptions menuDisplay_getPersistValuesOptions() {
    return persistValuesOptions;
}

NetworkOptions menuDisplay_getNetworkOptions() {
    return networkOptions;
}

BossRushOptions menuDisplay_getBossRushOptions() {
    return bossRushOptions;
}

int menuDisplay_bossRushUsesNoTriggers() {
    if (bossRushOptions.switchTriggers.bossHit == 0
        && bossRushOptions.switchTriggers.land == 0
        && bossRushOptions.switchTriggers.ring == 0
        && bossRushOptions.switchTriggers.networkBossHit == 0) 
    {
        return 1;
    }
    return 0;
}

NinesChallengeOptions menuDisplay_getNinesChallengeOptions() {
    return ninesChallengeOptions;
}

void switchToRandomAllowedGame() {
    if (terminalActiveRules == TERMINAL_RULSET_SHUFFLER || terminalActiveRules == TERMINAL_RULSET_SHUFFLER_WITH_VRAM) {        
        gameSuiteSelectIndex = rand() % 7;
        chooseGameSuite();
        cartLoader_loadRandomRom();
    } else {
        applyAllowedGamesForCurrentTerminalSelection();

        int totalGames = 0;
        for (int i = 0; i < 16; i++) {
            if (allowedGamesThisTerminal[i] == -1) {
                totalGames = i;
                break;
            }
        }

        if (totalGames > 1) {
            int currentGameId = cartLoader_getActiveCartIndex();
            int nextGameId = currentGameId;

            while (nextGameId == currentGameId) {
                nextGameId = allowedGamesThisTerminal[rand() % totalGames];
            }

            cartLoader_setAllGamesAsBlocked();
            cartLoader_unblockGamesWithCartNumber(nextGameId);
            cartLoader_loadRandomRom();
        }
    }
}

void addGameToThoseAllowedForTerminal(int gameIndex, int withGap) {
    int index = 0;
    for (int i = 0; i < 16; i++) {
        if (allowedGamesThisTerminal[i] == -1) {
            index = i;
            break;;
        }
    }

    allowedGamesThisTerminal[index] = gameIndex;
    spacesUnderGamesThisTerminal[index] = withGap;
}

void applyAllowedGamesForCurrentTerminalSelection() {
    clearAllowedGamesThisTerminal();

    if (terminalActiveRules == TERMINAL_RULSET_RINGS_MAKE_FASTER) {
        addGameToThoseAllowedForTerminal(1, 1);
        addGameToThoseAllowedForTerminal(2, 1);
        addGameToThoseAllowedForTerminal(3, 1);
        addGameToThoseAllowedForTerminal(4, 0);
    }

    if (terminalActiveRules == TERMINAL_RULSET_RINGS_CORRUPT_LEVEL) {
        // sonic MD
        addGameToThoseAllowedForTerminal(1, 0);
        addGameToThoseAllowedForTerminal(2, 0);
        addGameToThoseAllowedForTerminal(3, 0);
        addGameToThoseAllowedForTerminal(4, 1);
        // sonic SMS - I have added support for this!!
        addGameToThoseAllowedForTerminal(9, 0);
        addGameToThoseAllowedForTerminal(8, 0);
        addGameToThoseAllowedForTerminal(10, 0);
    }

    if (terminalActiveRules == TERMINAL_RULSET_RINGS_CORRUPT_RAM
        || terminalActiveRules == TERMINAL_RULSET_REMOVE_COLOUR
        || terminalActiveRules == TERMINAL_RULSET_NO_SPRITES_ALT
        || terminalActiveRules == TERMINAL_RULSET_NO_BACKGROUNDS_ALT
        || terminalActiveRules == TERMINAL_RULSET_SORT_COLOURS) {
        // sonic MD
        addGameToThoseAllowedForTerminal(1, 0);
        addGameToThoseAllowedForTerminal(2, 0);
        addGameToThoseAllowedForTerminal(3, 0);
        addGameToThoseAllowedForTerminal(4, 1);
        // sonic SMS
        addGameToThoseAllowedForTerminal(9, 0);
        addGameToThoseAllowedForTerminal(8, 0);
        addGameToThoseAllowedForTerminal(10, 0);
        // 3D Blast
        addGameToThoseAllowedForTerminal(6, 1);
        //mean bean
        addGameToThoseAllowedForTerminal(18, 0);
        // gunstar heroes
        addGameToThoseAllowedForTerminal(34, 0);
        // // revenge of shinobi
        addGameToThoseAllowedForTerminal(25, 0);
        // ecco
        addGameToThoseAllowedForTerminal(38, 0);
        // micro machines 2
        addGameToThoseAllowedForTerminal(27, 0);
    }

    if (terminalActiveRules == TERMINAL_RULSET_CONTROLLER) {
        // sonic MD
        addGameToThoseAllowedForTerminal(1, 0);
        addGameToThoseAllowedForTerminal(2, 0);
        addGameToThoseAllowedForTerminal(3, 0);
        addGameToThoseAllowedForTerminal(4, 1);
        // sonic SMS
        addGameToThoseAllowedForTerminal(8, 0);
        addGameToThoseAllowedForTerminal(9, 0);
        addGameToThoseAllowedForTerminal(10, 0);
        // triple trouble
        // addGameToThoseAllowedForTerminal(16, 1);
        //mean bean
        addGameToThoseAllowedForTerminal(18, 0);
    }
}

void clearAllowedGamesThisTerminal() {
    for (int i = 0; i < 16; i++) {
        allowedGamesThisTerminal[i] = -1;
        spacesUnderGamesThisTerminal[i] = 0;
    }
}


void menuDisplay_applyPresetRules(int rulesIndex) {
    activeTerminalRuleId = rulesIndex;

    applyDefaultPersistValues();
    applyDefaultRamDetectiveValues();
    applyDefaultSettings();    
    applySecondaryHacksDefaultValues();
    applyNetworkOptionsDefaultValues();
    applyDefaultBossRushValues();
    applyDefaultNinesChallengeValues();
    vdp_healAllColours();
    abortAllBossRushSettings();

    // FOR ALISTAIR
    // hackOptions.shouldWriteToLog = 1;

    networkOptions.allowSoloEffectswhenNetworked = 1;
    networkOptions.networkingIsActive = 1;
    secondaryHackOptions.screenSnapOnGetRing = 1;
    secondaryHackOptions.shouldSaveRewindStates = 1;
    hackOptions.switchGameType = 0;

    networkOptions.sendRandomiseVelocity = 0;
    networkOptions.sendRemoveColour = 0;
    networkOptions.sendSpeedUp = 0;
    networkOptions.sendSwitchGame = 0;
    networkOptions.sendWriteIntoLevelDifficulty = 0;

    secondaryHackOptions.colourDeleteAffectsAudio = 1;
    hackOptions.colourDeleteHealRate = 6;

    // now activate what's specific to each rule
    if (rulesIndex == 0) {
        // do nothing!
    }
    if (rulesIndex == 1) {
        hackOptions.switchGameType = 1;
    }
    if (rulesIndex == 2) {
        hackOptions.speedUpOnRing = 1;
    }
    if (rulesIndex == 3) {
        // overwrite level medium (from terminal)
        hackOptions.overwriteLevelType = 1;
        hackOptions.overwriteLevelDifficulty = 1;
    }
    if (rulesIndex == 4) {
        hackOptions.randomiseVelocityOnRing = 1;
    }
    if (rulesIndex == 5) {
        // overwrite RAM medium (from terminal)
        secondaryHackOptions.ramWritesPerRing = 3;
    }
    if (rulesIndex == 6) {
        // scramble VRAM
        secondaryHackOptions.vramWritesPerRing = 4;
    }
    if (rulesIndex == 7) {
        hackOptions.switchGameType = 1;
        hackOptions.copyVram = 1;
    }
    if (rulesIndex == 8) {
        hackOptions.colourDeleteTrigger = 1;
        hackOptions.colourDeleteHealRate = 6;
        hackOptions.colourDeletePattern = 0;
        secondaryHackOptions.colourDeleteAffectsAudio = 1;
    }
    if (rulesIndex == 9) {
        hackOptions.limitedColourType = 1 + (rand() % 4);
    }
    if (rulesIndex == 10) {
        hackOptions.shouldHideLayers = 2;
    }
    if (rulesIndex == 11) {
        hackOptions.shouldHideLayers = 1;
    }
    if (rulesIndex == 12) {
        hackOptions.shouldSortColours = 1;
    }
    if (rulesIndex == 13) {
        // overwrite level hard (from web)
        hackOptions.overwriteLevelType = 1;
        hackOptions.overwriteLevelDifficulty = 2;
    }
    if (rulesIndex == 14) {
        // overwrite RAM hard (from web)
        secondaryHackOptions.ramWritesPerRing = 3;
    }
    if (rulesIndex == 15) {
        // boss rush!
        applyDefaultBossRushValues();
        setStartBossRush(1);
    }

    modConsole_applyHackOptions();
    fireSnapEffect(1);
}

void menuDisplay_showTerminalMenu() {
    resetRotorChanges();
    endIdleMode();
    vdp_clearGraphicLayer(2);

    if (hasMappedRomsToLevels != 1) {
        mapBossRushesToRoms();
        hasMappedRomsToLevels = 1;
    }
    terminalLocationIndex = 2;
    setShouldUseControlsShuffle(0);
    menuDisplay_showMenu(MENU_LISTING_TERMINAL);
    setShouldCheckForIdleMode(0);

    // quick fix because boss rush doesn't go away properly
    bossRushOptions.orderSeed[0] = rand() % 0x10;
    bossRushOptions.orderSeed[1] = rand() % 0x10;
    bossRushOptions.orderSeed[2] = rand() % 0x10;
    bossRushOptions.orderSeed[3] = rand() % 0x10;
}

void menuDisplay_sendNinesSeedToOpponent() {
    ninesChallengeOptions.sentSeedToOpponent = 1;

    NinesChallengeOptions ninesOptions = menuDisplay_getNinesChallengeOptions();
    char message[0x100];
    sprintf(message, "%02d%02d%02d%02dx", ninesOptions.orderSeed[0], ninesOptions.orderSeed[1], ninesOptions.orderSeed[2], ninesOptions.orderSeed[3]);

    cartLoader_writeActionToNetwork(message);
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

    message[9] = networkOptions.sendRemoveColour != 0 ? NETWORK_MSG_INTERPRET_AS_POSITIVE : NETWORK_MSG_INTERPRET_AS_NEGATIVE;
    message[10] = NETWORK_MSG_REMOVE_COLOUR;
    if (networkOptions.sendRemoveColour == 1) {
        message[10] = NETWORK_MSG_REMOVE_COLOUR;
    }
    if (networkOptions.sendRemoveColour == 2) {
        message[10] = NETWORK_MSG_REMOVE_10_COLOURS;
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

    if (command == NETWORK_MSG_REMOVE_COLOUR) {
        if (asPositive == 0) {
            networkOptions.sendRemoveColour = 0;
        } else {
            networkOptions.sendRemoveColour = 1;
        }
    }
    if (command == NETWORK_MSG_REMOVE_10_COLOURS) {
        if (asPositive == 0) {
            networkOptions.sendRemoveColour = 0;
        } else {
            networkOptions.sendRemoveColour = 2;
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

int menuDisplay_shouldRamEditingOptionsShowAsOn() {
    if (secondaryHackOptions.ramWritesPerRing != 0) {
        return 1;
    }
    return 0;
}

int menuDisplay_shouldQualityOfLifeOptionsShowAsOn() {
    if (hackOptions.infiniteLives != 0 || hackOptions.infiniteTime != 0 || hackOptions.shouldWriteToLog != 0 || secondaryHackOptions.shouldSaveRewindStates != 0) {
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
    if (hackOptions.shouldSortColours != 0 || hackOptions.limitedColourType != 0 || hackOptions.copyVram != 0 || hackOptions.shouldHideLayers != 0 || hackOptions.colourDeleteTrigger != 0) {
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
    
    FILE *secondaryPrefsReader = fopen("_magicbox/__secondaryPrefs.data", "rb");

    if (secondaryPrefsReader) {
        int prefsBuffer[0x100];
        fread(prefsBuffer, sizeof(int), 0x100, secondaryPrefsReader);
        fclose(prefsReader);
        applySecondaryHacksFromArray256(prefsBuffer);
    } else {
        applySecondaryHacksDefaultValues();
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

    FILE *startupHintReader = fopen("_magicbox/__startupHint.data", "rb");
    if (startupHintReader) {
        fclose(startupHintReader);
        dismissStartupHint(0);
    }

    applyDefaultBossRushValues();
    applyDefaultNinesChallengeValues();

    saveHackOptions();
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

                sprintf(text, "%04X:%02X", loc, value);// (value >> 1) & 1);
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

void applyDefaultBossRushValues() {
    bossRushOptions.bossOrder = 0;

    bossRushOptions.switchTriggers.bossHit = 1;
    bossRushOptions.switchTriggers.ring = 0;
    bossRushOptions.switchTriggers.land = 0;
    bossRushOptions.switchTriggers.networkBossHit = 0;

    bossRushOptions.totalBossesIdx = 2;
    bossRushOptions.ringsOff = 0;
    bossRushOptions.showProgress = 1;
    bossRushOptions.carryRingsAcrossGames = 1;
    bossRushOptions.preventCarryInDoomsday = 1;

    bossRushOptions.orderSeed[0] = rand() % 0x10;
    bossRushOptions.orderSeed[1] = rand() % 0x10;
    bossRushOptions.orderSeed[2] = rand() % 0x10;
    bossRushOptions.orderSeed[3] = rand() % 0x10;
    bossRushOptions.seedEditingLocationIndex = 0;
    bossRushOptions.shouldRevealSeed = 0;
    bossRushOptions.shouldExposeTrackerData = 1;
    bossRushOptions.shouldUseExternalMusic = 0;

    bossRushOptions.didEditSeed = 0;
}

void applyDefaultNinesChallengeValues() {
    ninesChallengeOptions.shouldUseAllGames = 1;
    ninesChallengeOptions.shouldUseRandomOrder = 1;
    ninesChallengeOptions.orderSeed[0] = rand() % 0x10;
    ninesChallengeOptions.orderSeed[1] = rand() % 0x10;
    ninesChallengeOptions.orderSeed[2] = rand() % 0x10;
    ninesChallengeOptions.orderSeed[3] = rand() % 0x10;
    ninesChallengeOptions.didEditSeed = 0;
    ninesChallengeOptions.shouldRevealSeed = 0;
    ninesChallengeOptions.showProgress = 1;

    ninesChallengeOptions.allowTacticalDeaths = 1;
    ninesChallengeOptions.quitOnRingLoss = 0;
    ninesChallengeOptions.shouldUseCheckpoints = 1;

    ninesChallengeOptions.useOnlineRace = 0;
    ninesChallengeOptions.sentSeedToOpponent = 0;
    ninesChallengeOptions.receivedSeedFromOpponent = 0;
    ninesChallengeOptions.targetTotalIndex = 0;
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

void applySecondaryHacksDefaultValues() {
    secondaryHackOptions.colourDeleteAffectsAudio = 0;
    secondaryHackOptions.screenSnapOnGetRing = 1;
    secondaryHackOptions.ramWritesPerRing = 0;
    secondaryHackOptions.ramWriteStartLoc[0] = 0;
    secondaryHackOptions.ramWriteStartLoc[1] = 0;
    secondaryHackOptions.ramWriteStartLoc[2] = 0;
    secondaryHackOptions.ramWriteStartLoc[3] = 0;
    secondaryHackOptions.ramWriteEndLoc[0] = 0xF;
    secondaryHackOptions.ramWriteEndLoc[1] = 0xF;
    secondaryHackOptions.ramWriteEndLoc[2] = 0xF;
    secondaryHackOptions.ramWriteEndLoc[3] = 0xF;
    secondaryHackOptions.shouldSaveRewindStates = 0;
    secondaryHackOptions.vramWritesPerRing = 0;
    secondaryHackOptions.eventCountForSwitch = 0;
}

void applySecondaryHacksFromArray256(int array256[]) {
    secondaryHackOptions.colourDeleteAffectsAudio = array256[0];
    secondaryHackOptions.screenSnapOnGetRing = array256[1];
    secondaryHackOptions.ramWritesPerRing = array256[2];

    secondaryHackOptions.ramWriteStartLoc[0] = array256[3];
    secondaryHackOptions.ramWriteStartLoc[1] = array256[4];
    secondaryHackOptions.ramWriteStartLoc[2] = array256[5];
    secondaryHackOptions.ramWriteStartLoc[3] = array256[6];

    secondaryHackOptions.ramWriteEndLoc[0] = array256[7];
    secondaryHackOptions.ramWriteEndLoc[1] = array256[8];
    secondaryHackOptions.ramWriteEndLoc[2] = array256[9];
    secondaryHackOptions.ramWriteEndLoc[3] = array256[10];

    secondaryHackOptions.shouldSaveRewindStates = array256[11];
    secondaryHackOptions.vramWritesPerRing = array256[12];
    secondaryHackOptions.eventCountForSwitch = array256[13];
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

    hackOptions.colourDeleteTrigger = array256[17];
    hackOptions.colourDeletePattern = array256[18];
    hackOptions.colourDeleteHealRate = array256[19];

    hackOptions.shouldShowDeathCount = array256[20];
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

    hackOptions.colourDeleteTrigger = 0;
    hackOptions.colourDeletePattern = 0;
    hackOptions.colourDeleteHealRate = 0;

    hackOptions.shouldShowDeathCount = 0;

    // saveHackOptions();
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

    options[17] = hackOptions.colourDeleteTrigger;
    options[18] = hackOptions.colourDeletePattern;
    options[19] = hackOptions.colourDeleteHealRate;

    options[20] = hackOptions.shouldShowDeathCount;
    
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



    int secondaryPrefs[0x100];
    for (int i = 0; i < 0x100; i++) {
        secondaryPrefs[i] = 0;
    }
    secondaryPrefs[0] = secondaryHackOptions.colourDeleteAffectsAudio;
    secondaryPrefs[1] = secondaryHackOptions.screenSnapOnGetRing;
    secondaryPrefs[2] = secondaryHackOptions.ramWritesPerRing;

    secondaryPrefs[3] = secondaryHackOptions.ramWriteStartLoc[0];
    secondaryPrefs[4] = secondaryHackOptions.ramWriteStartLoc[1];
    secondaryPrefs[5] = secondaryHackOptions.ramWriteStartLoc[2];
    secondaryPrefs[6] = secondaryHackOptions.ramWriteStartLoc[3];
    
    secondaryPrefs[7] = secondaryHackOptions.ramWriteEndLoc[0];
    secondaryPrefs[8] = secondaryHackOptions.ramWriteEndLoc[1];
    secondaryPrefs[9] = secondaryHackOptions.ramWriteEndLoc[2];
    secondaryPrefs[10] = secondaryHackOptions.ramWriteEndLoc[3];

    secondaryPrefs[11] = secondaryHackOptions.shouldSaveRewindStates;

    secondaryPrefs[13] = secondaryHackOptions.eventCountForSwitch;

    remove("_magicbox/__secondaryPrefs.data");
    FILE *secondaryPrefsWriter = fopen("_magicbox/__secondaryPrefs.data", "wb");

    for (int i = 0; i < 0x100; i++) {
        fwrite(secondaryPrefs, sizeof(int), 0x100, secondaryPrefsWriter);
    }
    fclose(secondaryPrefsWriter);


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
    // char tempLog[256];
    // sprintf(tempLog, "menuDisplay_showMenu %d", menuNum);
    // cartLoader_appendToLog(tempLog);

    // // the seed has been revealed by the end-screen at this stage, so make sure
    // // that is reflected in the stats and menus
    // if (shouldUseBossRush() == 1 && getBossRushComplete() == 1) {
    //     bossRushOptions.shouldRevealSeed = 1;
    // }

    activeMenu = menuNum;
    vdp_setShouldRandomiseColours(1);
    aa_psg_mute();
    aa_ym2612_mute();
    aa_ym2413_mute();

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

    if (activeMenu == MENU_LISTING_BOSS_RUSH) {
        showBossRushMenu(); 
    }
    
    if (activeMenu == MENU_LISTING_NINES_CHALLENGE) {
        showNinesChallengeMenu(); 
    }
    
    if (activeMenu == MENU_LISTING_BOSS_RUSH_TRIGGER_SELECT) {
        showBossRushTriggerSelectMenu(); 
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
    
    if (activeMenu == MENU_LISTING_RAM_EDITING) {
        showRamEditingOptionsMenu(); 
    }

    if (activeMenu == MENU_LISTING_TERMINAL) {
        showTerminalMenu();
    }

    if (activeMenu == MENU_LISTING_TERMINAL_SHUFFLER) {
        showTerminalShufflerSelectMenu();
    }

    if (activeMenu == MENU_LISTING_TERMINAL_GAME_LIST) {
        showTerminalGameListMenu();
    }
}

void menuDisplay_hideMenu() {
    activeMenu = MENU_LISTING_NONE;

    vdp_setShouldRandomiseColours(0); // this should only be zeroed once!!
    aa_psg_unmute();
    aa_ym2612_unmute();
    aa_ym2413_unmute();

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
    aa_ym2413_unmute();
    cartLoader_applyHackOptions(gameHasStarted);
    modConsole_applyHackOptions();
    modConsole_applyNetworkOptions();
    cartLoader_loadRomAtIndex(chosenGameIndex, 0);

    gameHasStarted = 1;
}

int getRequestedNinesChallengeStartRom() {
    return requestedNinesChallengeStartRom;
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
            if (awaitingBossRushStart()) {
                beginGame();
                beginBossRush();
            } else if (awaitingNinesChallengeStart()) {
                beginGame();
                requestedNinesChallengeStartRom = chosenGameIndex;
                beginNinesChallenge();
            } else {
                beginGame();
            }
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
            aa_ym2413_unmute();

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
    
    if (activeMenu == MENU_LISTING_RAM_EDITING) {
        if (buttonIndex == INPUT_INDEX_UP) {
            ramEditingOptionsIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            ramEditingOptionsIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_LEFT) {
            incrementRamEditingOptionWithDPad(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_RIGHT) {
            incrementRamEditingOptionWithDPad(1);
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B) {
            incrementRamEditingOptionWithFaceButton(-1);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C) {
            incrementRamEditingOptionWithFaceButton(1);
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_NETWORKING) {
        if (buttonIndex == INPUT_INDEX_UP) {
            networkingOptionsIndex--;
            while (networkingOptionsIndex == 2 || networkingOptionsIndex == 3 || networkingOptionsIndex == 4 || networkingOptionsIndex == 5 || networkingOptionsIndex == 11 || networkingOptionsIndex == 13) {
                networkingOptionsIndex --;
            }
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            networkingOptionsIndex++;
            while (networkingOptionsIndex == 2 || networkingOptionsIndex == 3 || networkingOptionsIndex == 4 || networkingOptionsIndex == 5 || networkingOptionsIndex == 11 || networkingOptionsIndex == 13) {
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

    if (activeMenu == MENU_LISTING_TERMINAL) {
        if (buttonIndex == INPUT_INDEX_UP) {
            terminalLocationIndex--;
            if (terminalLocationIndex == 8) {
                terminalLocationIndex--;
            }
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            terminalLocationIndex++;
            if (terminalLocationIndex == 8) {
                terminalLocationIndex++;
            }
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_START) {
            if (terminalLocationIndex < 14) {
                enterTerminalOption();
            } else {
                if (terminalActiveRules == TERMINAL_RULSET_CONTROLLER) {
                    setShouldUseControlsShuffle(1);
                }
                vdp_setShouldRandomiseColours(0);
                cartLoader_applyHackOptions(gameHasStarted);
                modConsole_applyHackOptions();
                modConsole_applyNetworkOptions();

                menuDisplay_hideMenu();
                setShouldCheckForIdleMode(1);
            }
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_TERMINAL_SHUFFLER) {
        if (buttonIndex == INPUT_INDEX_UP) {
            gameSuiteSelectIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            gameSuiteSelectIndex++;
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_START) {
            if (gameSuiteSelectIndex < 9) {
                chooseGameSuite();

                int ruleset = 1;
                if (terminalActiveRules == TERMINAL_RULSET_SHUFFLER_WITH_VRAM) {
                    ruleset = 7;
                }
                // modConsole_activateReset();
                // cartLoader_cacheSaveStateBeforeMenu();

                menuDisplay_applyPresetRules(ruleset);
                cartLoader_applyHackOptions(gameHasStarted);
                modConsole_applyHackOptions();
                modConsole_applyNetworkOptions();
                cartLoader_clearSaveStates();
                cartLoader_loadRandomRom();
                modConsole_activateReset();
                vdp_setShouldRandomiseColours(0);

                cartLoader_loadCurrentStartupStateFromDisk();
                cartLoader_cacheSaveStateBeforeMenu();
                menuDisplay_hideMenu();
                setShouldCheckForIdleMode(1);
            } else {
                menuDisplay_showMenu(MENU_LISTING_TERMINAL);
            }
            return 1;
        }
    }

        if (activeMenu == MENU_LISTING_TERMINAL_GAME_LIST) {
        if (buttonIndex == INPUT_INDEX_UP) {
            gameSuiteSelectIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            gameSuiteSelectIndex++;
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_START) {
            if (gameSuiteSelectIndex < gameCountThisTerminal) {
                initialiseChosenTerminalGame();

                vdp_setShouldRandomiseColours(0);
                cartLoader_applyHackOptions(gameHasStarted);
                modConsole_applyHackOptions();
                modConsole_applyNetworkOptions();

                cartLoader_cacheSaveStateBeforeMenu();
                menuDisplay_hideMenu();
                setShouldCheckForIdleMode(1);
            } else {
                menuDisplay_showMenu(MENU_LISTING_TERMINAL);
            }
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_BOSS_RUSH) {
        if (buttonIndex == INPUT_INDEX_UP) {
            bossRushItemIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            bossRushItemIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementBossRushOption(-1, buttonIndex);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementBossRushOption(1, buttonIndex);
            refreshMenu();
            return 1;
        }
    }

    if (activeMenu == MENU_LISTING_NINES_CHALLENGE) {
        if (buttonIndex == INPUT_INDEX_UP) {
            ninesChallengeItemIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            ninesChallengeItemIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementNinesChallengeOption(-1, buttonIndex);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementNinesChallengeOption(1, buttonIndex);
            refreshMenu();
            return 1;
        }
    }
    
    if (activeMenu == MENU_LISTING_BOSS_RUSH_TRIGGER_SELECT) {
        if (buttonIndex == INPUT_INDEX_UP) {
            bossRushTriggerSelectItemIndex--;
            refreshMenu();
            return 1;
        }
        if (buttonIndex == INPUT_INDEX_DOWN) {
            bossRushTriggerSelectItemIndex++;
            refreshMenu();
            return 1;
        }

        if (buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_LEFT) {
            incrementBossRushTiggerSelectOption(-1, buttonIndex);
            refreshMenu();
            return 1;
        }
        
        if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_C || buttonIndex == INPUT_INDEX_RIGHT) {
            incrementBossRushTiggerSelectOption(1, buttonIndex);
            refreshMenu();
            return 1;
        }
    }


    return 0;
}

void initialiseChosenTerminalGame() {
    // modConsole_activateReset();
    // cartLoader_cacheSaveStateBeforeMenu();

    setShouldShuffleController(0);

    int effectIndexToActivate = 0;
    if (terminalActiveRules == TERMINAL_RULSET_RINGS_MAKE_FASTER) {
        effectIndexToActivate = 2;
    }
    if (terminalActiveRules == TERMINAL_RULSET_RINGS_CORRUPT_LEVEL) {
        effectIndexToActivate = 3;
    }
    if (terminalActiveRules == TERMINAL_RULSET_RINGS_CORRUPT_RAM) {
        effectIndexToActivate = 5;
    }
    if (terminalActiveRules == TERMINAL_RULSET_REMOVE_COLOUR) {
        effectIndexToActivate = 8;
    }
    if (terminalActiveRules == TERMINAL_RULSET_NO_SPRITES_ALT) {
        effectIndexToActivate = 11;
    }
    if (terminalActiveRules == TERMINAL_RULSET_NO_BACKGROUNDS_ALT) {
        effectIndexToActivate = 10;
    }
    if (terminalActiveRules == TERMINAL_RULSET_SORT_COLOURS) {
        effectIndexToActivate = 12;
    }
    if (terminalActiveRules == TERMINAL_RULSET_BOSS_RUSH) { // <-- this should never be hit!
        effectIndexToActivate = 0;
        // setShouldResetBossRush(1);
    }
    if (terminalActiveRules == TERMINAL_RULSET_CONTROLLER) {
        effectIndexToActivate = 0; // <-- need to support controller!
        setShouldUseControlsShuffle(1);
        gamepad_shuffleControls();
        setShouldShuffleController(1);
    }

    menuDisplay_applyPresetRules(effectIndexToActivate);
    cartLoader_setAllGamesAsBlocked();
    int gameIndex = allowedGamesThisTerminal[gameSuiteSelectIndex];

    // char tempLog2[256];
    // sprintf(tempLog2,"Picked allowedGamesThisTerminal[%i] is %i", gameSuiteSelectIndex, gameIndex);
    // cartLoader_appendToLog(tempLog2);

    cartLoader_unblockGamesWithCartNumber(gameIndex);

    cartLoader_applyHackOptions(gameHasStarted);
    modConsole_applyHackOptions();
    modConsole_applyNetworkOptions();
    cartLoader_clearSaveStates();
    cartLoader_loadRandomRom();

    modConsole_activateReset();
    cartLoader_loadCurrentStartupStateFromDisk();
    setShouldCheckForIdleMode(1);
}

void chooseGameSuite() {
    cartLoader_setAllGamesAsBlocked();

    if (gameSuiteSelectIndex == 0) {
        // sonic classics MD
        cartLoader_unblockGamesWithCartNumber(1);
        cartLoader_unblockGamesWithCartNumber(2);
        cartLoader_unblockGamesWithCartNumber(3);
        cartLoader_unblockGamesWithCartNumber(4);
    }

    if (gameSuiteSelectIndex == 1) {
        // sonic classics SMS
        cartLoader_unblockGamesWithCartNumber(8);
        cartLoader_unblockGamesWithCartNumber(9);
        cartLoader_unblockGamesWithCartNumber(10);
    }
    
    if (gameSuiteSelectIndex == 2) {
        // all sonic MD
        cartLoader_unblockGamesWithCartNumber(1);
        cartLoader_unblockGamesWithCartNumber(2);
        cartLoader_unblockGamesWithCartNumber(3);
        cartLoader_unblockGamesWithCartNumber(4);
        cartLoader_unblockGamesWithCartNumber(6);
        cartLoader_unblockGamesWithCartNumber(7);
        cartLoader_unblockGamesWithCartNumber(18);
    }

    if (gameSuiteSelectIndex == 3) {
        // puyo puyo
        cartLoader_unblockGamesWithCartNumber(18);
        cartLoader_unblockGamesWithCartNumber(19);
        cartLoader_unblockGamesWithCartNumber(20);
    }

    if (gameSuiteSelectIndex == 4) {
        // micro machines
        cartLoader_unblockGamesWithCartNumber(27);
        cartLoader_unblockGamesWithCartNumber(28);
        cartLoader_unblockGamesWithCartNumber(29);
    }

    if (gameSuiteSelectIndex == 5) {
        // streets of rage
        // cartLoader_unblockGamesWithCartNumber(30);
        // cartLoader_unblockGamesWithCartNumber(31);
        // cartLoader_unblockGamesWithCartNumber(32);
        // // also european/jp versions
        cartLoader_unblockGamesWithCartNumber(21);
        cartLoader_unblockGamesWithCartNumber(22);
        cartLoader_unblockGamesWithCartNumber(23);
    }
    
    if (gameSuiteSelectIndex == 6) {
        // shinobi
        cartLoader_unblockGamesWithCartNumber(24);
        cartLoader_unblockGamesWithCartNumber(25);
        cartLoader_unblockGamesWithCartNumber(26);
    }

    if (gameSuiteSelectIndex == 7) { // replace this with "up to 6 random"? Sometimes seems to just pick 2... duplicates? Games not found? Streets of rage wrong roms?
        // 4x rando
        int carts[4];
        int allowedCarts[26] = {1, 2, 3, 4, 6, 7, 18, 19, 20, 27, 28, 29, 21, 22, 23, 24, 25, 26, 8, 9, 10, 12, 13, 14, 15, 16};
        for (int i = 0; i < 4; i++) {
            carts[i] = -1;
        }
        int filledCarts = 0;

        while (filledCarts < 4) {
            int chosen = allowedCarts[rand() % 26];
            for (int i = 0; i < filledCarts; i++) {
                if (carts[i] == chosen) {
                    chosen = -1;
                }
            }

            if (chosen >= 0) {
                carts[filledCarts] = chosen;
                filledCarts++;
            }
        }

        for (int i = 0; i < 4; i++) {
            cartLoader_unblockGamesWithCartNumber(carts[i]);
        }
    }
}



void enterTerminalOption() {
    gameSuiteSelectIndex = 0;

    if (terminalLocationIndex == 2) {
        terminalActiveRules = TERMINAL_RULSET_SHUFFLER; 
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_SHUFFLER);
    } else if (terminalLocationIndex == 3) {
        terminalActiveRules = TERMINAL_RULSET_SHUFFLER_WITH_VRAM;
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_SHUFFLER);
    } else if (terminalLocationIndex == 4) {
        terminalActiveRules = TERMINAL_RULSET_RINGS_MAKE_FASTER;

        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    } else if (terminalLocationIndex == 5) {
        terminalActiveRules = TERMINAL_RULSET_RINGS_CORRUPT_LEVEL;
        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    } else if (terminalLocationIndex == 6) {
        terminalActiveRules = TERMINAL_RULSET_RINGS_CORRUPT_RAM;
        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    } else if (terminalLocationIndex == 7) {
        terminalActiveRules = TERMINAL_RULSET_REMOVE_COLOUR;
        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    } else if (terminalLocationIndex == 9) {
        terminalActiveRules = TERMINAL_RULSET_BOSS_RUSH;
        setShouldResetBossRush(1);

        menuDisplay_applyPresetRules(15);
        cartLoader_applyHackOptions(gameHasStarted);
        
        modConsole_applyHackOptions();
        modConsole_applyNetworkOptions();

        beginGame();
        beginBossRush();

        vdp_setShouldRandomiseColours(0);
        menuDisplay_hideMenu();
    } else if (terminalLocationIndex == 10) {
        terminalActiveRules = TERMINAL_RULSET_CONTROLLER;
        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    } else if (terminalLocationIndex == 11) {
        terminalActiveRules = TERMINAL_RULSET_SORT_COLOURS;
        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    } else if (terminalLocationIndex == 12) {
        terminalActiveRules = TERMINAL_RULSET_NO_BACKGROUNDS_ALT;
        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    } else if (terminalLocationIndex == 13) {
        terminalActiveRules = TERMINAL_RULSET_NO_SPRITES_ALT;
        applyAllowedGamesForCurrentTerminalSelection();
        menuDisplay_showMenu(MENU_LISTING_TERMINAL_GAME_LIST);
    }
}

void incrementBossRushTiggerSelectOption(int direction, int buttonIndex) {
    if (bossRushTriggerSelectItemIndex == 0) {
        bossRushOptions.switchTriggers.bossHit += direction;
    }
    if (bossRushTriggerSelectItemIndex == 1) {
        bossRushOptions.switchTriggers.ring += direction;
    }
    if (bossRushTriggerSelectItemIndex == 2) {
        bossRushOptions.switchTriggers.land += direction;
    }

    if (bossRushTriggerSelectItemIndex == 3) {
        bossRushOptions.switchTriggers.networkBossHit += direction;

        if (bossRushOptions.switchTriggers.networkBossHit == 1) {
            networkOptions.networkingIsActive = 1;
            networkOptions.sendRandomiseVelocity = 0;
            networkOptions.sendRemoveColour = 0;
            networkOptions.sendSpeedUp = 0;
            networkOptions.sendSwitchGame = 0;
            networkOptions.sendWriteIntoLevelDifficulty = 0;
            networkOptions.allowSoloEffectswhenNetworked = 1;
        }
    }


    if (bossRushTriggerSelectItemIndex == 4) {
        menuDisplay_showMenu(MENU_LISTING_BOSS_RUSH);
    }
}

void incrementNinesChallengeOption(int direction, int buttonIndex) {
    if (ninesChallengeItemIndex == 0) {
        toggleStartNinesChallenge();
    }
    if (ninesChallengeItemIndex == 1) {
        setShouldResetNinesChallenge(1 - getShouldResetNinesChallenge());

        if (getShouldResetNinesChallenge() == 1) {
            if (ninesChallengeOptions.shouldRevealSeed == 0) {
                shouldRerollNinesChallengeRandomTime = 30;
            }
        }
    }
    if (ninesChallengeItemIndex == 2) {
        ninesChallengeOptions.shouldUseAllGames += direction;
    }
    if (ninesChallengeItemIndex == 3) {
        ninesChallengeOptions.shouldUseRandomOrder += direction;
    }

    if (ninesChallengeItemIndex == 4) {
        if (ninesChallengeOptions.shouldRevealSeed == 0) {
            ninesChallengeOptions.shouldRevealSeed = 1;
        } else {
            if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_C) {
                ninesChallengeOptions.orderSeed[ninesChallengeOptions.seedEditingLocationIndex] += direction;
                ninesChallengeOptions.didEditSeed = 1;
            } else {
                ninesChallengeOptions.seedEditingLocationIndex += direction;
            }
        }
    }

    if (ninesChallengeItemIndex == 5) {
        shouldRerollNinesChallengeRandomTime = 30;
    }
    if (ninesChallengeItemIndex == 6) {
        ninesChallengeOptions.allowTacticalDeaths += direction;
    }

    if (ninesChallengeItemIndex == 7) {
        ninesChallengeOptions.targetTotalIndex += direction;
    }

    if (ninesChallengeItemIndex == 8) {
        ninesChallengeOptions.shouldUseCheckpoints += direction;
    }

    if (ninesChallengeItemIndex == 9) {
        ninesChallengeOptions.useOnlineRace += direction;

        networkOptions.networkingIsActive = ninesChallengeOptions.useOnlineRace;
        networkOptions.allowSoloEffectswhenNetworked = 0;
        networkOptions.sendRemoveColour = 0;
        networkOptions.sendSpeedUp = 0;
        networkOptions.sendSwitchGame = 0;
        networkOptions.sendRandomiseVelocity = 0;
        networkOptions.sendWriteIntoLevelDifficulty = 0;
    }

    if (ninesChallengeItemIndex == 10) {
        char requestMsg[0x100];
        sprintf(requestMsg, "%c", NETWORK_MSG_REQUEST_OPPONENT_SEED);
        cartLoader_writeActionToNetwork(requestMsg);
    }

    
    if (ninesChallengeItemIndex == 11) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
}

void incrementBossRushOption(int direction, int buttonIndex) {
    if (bossRushItemIndex == 0) {
        toggleStartBossRush();
    }
    if (bossRushItemIndex == 1) {
        setShouldResetBossRush(1 - getShouldResetBossRush());

        if (getShouldResetBossRush() == 1) {
            if (bossRushOptions.shouldRevealSeed == 0) {
                shouldRerollBossRushRandomTime = 30;
            }
        }
    }
    if (bossRushItemIndex == 2) {
        menuDisplay_showMenu(MENU_LISTING_BOSS_RUSH_TRIGGER_SELECT);
    }
    if (bossRushItemIndex == 3) {
        bossRushOptions.bossOrder += direction;
    }    
    if (bossRushItemIndex == 4) {
        bossRushOptions.totalBossesIdx += direction;
    }

    if (bossRushItemIndex == 5) {
        bossRushOptions.ringsOff += direction;
    }

    if (bossRushItemIndex == 6) {
        bossRushOptions.carryRingsAcrossGames += direction;
    }
    if (bossRushItemIndex == 7) {
        bossRushOptions.preventCarryInDoomsday += direction;
    }
    if (bossRushItemIndex == 8) {
        if (bossRushOptions.shouldRevealSeed == 0) {
            bossRushOptions.shouldRevealSeed = 1;
        } else {
            if (buttonIndex == INPUT_INDEX_A || buttonIndex == INPUT_INDEX_B || buttonIndex == INPUT_INDEX_C) {
                bossRushOptions.orderSeed[bossRushOptions.seedEditingLocationIndex] += direction;
                bossRushOptions.didEditSeed = 1;
            } else {
                bossRushOptions.seedEditingLocationIndex += direction;
            }
        }
    }

    if (bossRushItemIndex == 9) {
        shouldRerollBossRushRandomTime = 30;
    }

    if (bossRushItemIndex == 10) {
        bossRushOptions.showProgress += direction;
    }
    if (bossRushItemIndex == 11) {
        bossRushOptions.shouldExposeTrackerData += direction;
    }
    if (bossRushItemIndex == 12) {
        bossRushOptions.shouldUseExternalMusic += direction;
    }

    if (bossRushItemIndex == 13) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
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
    if (networkingOptionsIndex == 10) {
        networkOptions.sendRemoveColour += direction;
    }

    if (networkingOptionsIndex == 12) {
        char action[0x100];
        sprintf(action, "");
        action[0] = NETWORK_MSG_REQUEST_RULES;
        cartLoader_writeActionToNetwork(action);

        networkOptions.awaitingOpponentSettingsState = 1;
    }


    if (networkingOptionsIndex == 14) {
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
        secondaryHackOptions.eventCountForSwitch += direction;
    }
    if (gameSwapOptionIndex == 3) {
        hackOptions.copyVram += direction;
    }
    if (gameSwapOptionIndex == 4) {
        hackOptions.swapOrder += direction;
    }
    if (gameSwapOptionIndex == 5) {
        hackOptions.shouldShowSwapCount += direction;
    }

    if (gameSwapOptionIndex == 6) {
        hackOptions.shouldShowDeathCount += direction;
    }

    if (gameSwapOptionIndex == 7) {
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
        secondaryHackOptions.shouldSaveRewindStates += direction;
    }


    if (qualityOfLifeOptionIndex == 4) {
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
        hackOptions.colourDeleteTrigger += direction;
    }
    if (visualsOptionIndex == 4) {
        hackOptions.colourDeletePattern += direction;
    }
    if (visualsOptionIndex == 5) {
        hackOptions.colourDeleteHealRate += direction;
    }

    if (visualsOptionIndex == 6) {
        secondaryHackOptions.colourDeleteAffectsAudio += direction;
    }

    if (visualsOptionIndex == 7) {
        secondaryHackOptions.screenSnapOnGetRing += direction;
    }


    if (visualsOptionIndex == 8) {
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
    aa_ym2413_unmute();

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
        vdp_healAllColours();
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

    if (inGameOptionIndex == 10) {
        vdp_healAllColours();
    }

    if (inGameOptionIndex == 11) {
        stepBackRewindRAM();
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
        networkingOptionsIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_RAM_EDITING);
    }

    if (optionsItemIndex == 7) {
        qualityOfLifeOptionIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_BOSS_RUSH);
    }
    
    if (optionsItemIndex == 8) {
        qualityOfLifeOptionIndex = 0;
        menuDisplay_showMenu(MENU_LISTING_NINES_CHALLENGE);
    }

    if (optionsItemIndex == 9) {
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

void incrementRamEditingOptionWithDPad(int direction) {
    if (ramEditingOptionsIndex == 0) {
        secondaryHackOptions.ramWritesPerRing += direction;
    }

    if (ramEditingOptionsIndex == 1 || ramEditingOptionsIndex == 2) {
        ramEditingLocationIndex += direction;
    }

    if (ramEditingOptionsIndex == 3) {
        secondaryHackOptions.shouldSaveRewindStates += direction;
    }
    
    if (ramEditingOptionsIndex == 4) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
    }
}

void incrementRamEditingOptionWithFaceButton(int direction) {
    if (ramEditingOptionsIndex == 0) {
        secondaryHackOptions.ramWritesPerRing += direction;
    }

    if (ramEditingOptionsIndex == 1) {
        secondaryHackOptions.ramWriteStartLoc[ramEditingLocationIndex] += direction;
    }

    if (ramEditingOptionsIndex == 2) {
        secondaryHackOptions.ramWriteEndLoc[ramEditingLocationIndex] += direction;
    }

    if (ramEditingOptionsIndex == 3) {
        secondaryHackOptions.shouldSaveRewindStates += direction;
    }

    if (ramEditingOptionsIndex == 4) {
        menuDisplay_showMenu(MENU_LISTING_SETTINGS);
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

    showVersionNumber();
}

void showVersionNumber() {
    char versionText[0x100];
    sprintf(versionText, "V%d.%02d", majorVersion, minorVersion);
    layerRenderer_writeWord256Centred(0, 7 * DEFAULT_WIDTH / 8, DEFAULT_HEIGHT - 28, versionText, 5);
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

    showVersionNumber();
}

void showOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "options", 5);

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

    if (menuDisplay_shouldGameSwapOptionsShowAsOn() != 0) {
        if (networkOptions.allowSoloEffectswhenNetworked != 0) {
            sprintf(lines[0], "[BLOCKED] Game swapping >");
        } else {
            sprintf(lines[0], "[ON] Game swapping >");
        }
    } else {
        sprintf(lines[0], "     Game swapping >");
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
        sprintf(lines[5], "[ON] Networking/Twitch >");
    } else {
        sprintf(lines[5], "     Networking/Twitch >");
    }

    if (menuDisplay_shouldRamEditingOptionsShowAsOn() != 0) {
        sprintf(lines[6], "[ON] RAM Editing >");
    } else {
        sprintf(lines[6], "     RAM Editing >");
    }

    if (shouldUseBossRush() || awaitingBossRushStart()) {
        sprintf(lines[7], "[ON] Boss Rush (Beta)>");
    } else {
        sprintf(lines[7], "     Boss Rush (Beta)>");
    }

    if (shouldUseNinesChallenge() || awaitingNinesChallengeStart()) {
        sprintf(lines[8], "[ON] %s Challenge (Beta)>", getNinesChallengeTarget());
    } else {
        sprintf(lines[8], "     %s Challenge (Beta)>", getNinesChallengeTarget());
    }

    sprintf(lines[9], "Start game");

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

    showVersionNumber();
}

void showInGameOptionsMenu() {
    // cartLoader_appendToLog("showInGameOptionsMenu");

    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "options", 5);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 32, "--- press A/B/C to activate option ---", 5);


    char titleText[0x100];
    sprintf(titleText, "----- Alistair's Magic Box V%d.%02d -----", majorVersion, minorVersion);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT - 24, titleText, 5);

    int lineCount = 13;
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
    sprintf(lines[10], "Heal all lost colours");
    sprintf(lines[11], "Rewind game state");
    if (inGameOptionIndex == 11) {
        sprintf(hintLines[0], "* You can also press LEFT + B + START");
        sprintf(hintLines[1], "  In-game to rewind in 2-second steps");
    }
    sprintf(lines[12], "Back to game");

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

    int lineCount = 8;
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

    if (hackOptions.switchGameType > 6) {
        hackOptions.switchGameType = 0;
    }
    if (hackOptions.switchGameType < 0) {
        hackOptions.switchGameType = 6;
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
    } else if (hackOptions.switchGameType == 5) {
        sprintf(lines[0], "Switch games:        ON LAND");
    } else if (hackOptions.switchGameType == 6) {
        sprintf(lines[0], "Switch games:   ON RING/BOSS");
    }

    if (hackOptions.cooldownOnSwitch > 6) {
        hackOptions.cooldownOnSwitch = 0;
    }
    if (hackOptions.cooldownOnSwitch < 0) {
        hackOptions.cooldownOnSwitch = 6;
    }
    if (hackOptions.cooldownOnSwitch == 0) {
        sprintf(lines[1], "Cooldown after switch:   OFF");
    } else if (hackOptions.cooldownOnSwitch == 1) {
        sprintf(lines[1], "Cooldown after switch: 0.25 sec");
    } else if (hackOptions.cooldownOnSwitch == 2) {
        sprintf(lines[1], "Cooldown after switch: 0.50 sec");
    } else if (hackOptions.cooldownOnSwitch == 3) {
        sprintf(lines[1], "Cooldown after switch: 1.00 sec");
    } else if (hackOptions.cooldownOnSwitch == 4) {
        sprintf(lines[1], "Cooldown after switch: 2.50 sec");
    } else if (hackOptions.cooldownOnSwitch == 5) {
        sprintf(lines[1], "Cooldown after switch: 5.00 sec");
    } else if (hackOptions.cooldownOnSwitch == 6) {
        sprintf(lines[1], "Cooldown after switch: 15 sec");
    }

    if (secondaryHackOptions.eventCountForSwitch > 3) {
        secondaryHackOptions.eventCountForSwitch = 0;
    }
    if (secondaryHackOptions.eventCountForSwitch < 0) {
        secondaryHackOptions.eventCountForSwitch = 3;
    }
    if (secondaryHackOptions.eventCountForSwitch == 0) {
        sprintf(lines[2], "Events needed to switch:   1");
    } else if (secondaryHackOptions.eventCountForSwitch == 1) {
        sprintf(lines[2], "Events needed to switch:   2");
    } else if (secondaryHackOptions.eventCountForSwitch == 2) {
        sprintf(lines[2], "Events needed to switch:   5");
    } else if (secondaryHackOptions.eventCountForSwitch == 3) {
        sprintf(lines[2], "Events needed to switch:  10");
    }


    if (hackOptions.copyVram > 4) {
        hackOptions.copyVram = 0;
    }
    if (hackOptions.copyVram < 0) {
        hackOptions.copyVram = 3;
    }
    if (hackOptions.copyVram == 0) {
        sprintf(lines[3], "Keep vram on switch:     OFF");
    } else if (hackOptions.copyVram == 1) {
        sprintf(lines[3], "Keep vram on switch:   100%%");
    } else if (hackOptions.copyVram == 2) {
        sprintf(lines[3], "Keep vram on switch:    50%%");
    } else if (hackOptions.copyVram == 3) {
        sprintf(lines[3], "Keep vram on switch:    10%%");
    } else if (hackOptions.copyVram == 4) {
        sprintf(lines[3], "Keep vram on switch:     1%%");
    }

    if (hackOptions.swapOrder > 1) {
        hackOptions.swapOrder = 0;
    }
    if (hackOptions.swapOrder < 0) {
        hackOptions.swapOrder = 3;
    }
    if (hackOptions.swapOrder == 0) {
        sprintf(lines[4], "Swap order:           random");
    } else {
        sprintf(lines[4], "Swap order:     alphabetical");
    }
    
    if (hackOptions.shouldShowSwapCount > 1) {
        hackOptions.shouldShowSwapCount = 0;
    }
    if (hackOptions.shouldShowSwapCount < 0) {
        hackOptions.shouldShowSwapCount = 1;
    }
    if (hackOptions.shouldShowSwapCount == 0) {
        sprintf(lines[5], "Show swap counter:       OFF");
    } else {
        sprintf(lines[5], "Show swap counter:        ON");
    }

    if (hackOptions.shouldShowDeathCount > 1) {
        hackOptions.shouldShowDeathCount = 0;
    }
    if (hackOptions.shouldShowDeathCount < 0) {
        hackOptions.shouldShowDeathCount = 1;
    }
    if (hackOptions.shouldShowDeathCount == 0) {
        sprintf(lines[6], "Show death counter:      OFF");
    } else {
        sprintf(lines[6], "Show death counter:       ON");
    }


    sprintf(lines[7], "back >");

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

    int lineCount = 5;
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

    if (secondaryHackOptions.shouldSaveRewindStates > 1) {
        secondaryHackOptions.shouldSaveRewindStates = 0;
    }
    if (secondaryHackOptions.shouldSaveRewindStates < 0) {
        secondaryHackOptions.shouldSaveRewindStates = 1;
    }
    if (secondaryHackOptions.shouldSaveRewindStates == 0) {
        sprintf(lines[3], "Allow game rewind:       OFF");
    } else {
        sprintf(lines[3], "Allow game rewind:        ON"); 
    }

    sprintf(lines[4], "back >");

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

    int lineCount = 9;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
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
    linesWithBreakAfter[2] = 1;

    if (hackOptions.colourDeleteTrigger > 5) {
        hackOptions.colourDeleteTrigger = 0;
    }
    if (hackOptions.colourDeleteTrigger < 0) {
        hackOptions.colourDeleteTrigger = 5;
    }
    if (hackOptions.colourDeleteTrigger == 0) {
        sprintf(lines[3], "Remove colour:             OFF");
    } else if (hackOptions.colourDeleteTrigger == 1) {
        sprintf(lines[3], "Remove colour:     ON GET RING");
    } else if (hackOptions.colourDeleteTrigger == 2) {
        sprintf(lines[3], "Remove colour: 10x ON GET RING");
    } else if (hackOptions.colourDeleteTrigger == 3) {
        sprintf(lines[3], "Remove colour:     10x per SEC");
    } else if (hackOptions.colourDeleteTrigger == 4) {
        sprintf(lines[3], "Remove colour:      1x per SEC");
    } else if (hackOptions.colourDeleteTrigger == 5) {
        sprintf(lines[3], "Remove colour:  1x per 10 SECS");
    }
    
    if (hackOptions.colourDeletePattern > 2) {
        hackOptions.colourDeletePattern = 0;
    }
    if (hackOptions.colourDeletePattern < 0) {
        hackOptions.colourDeletePattern = 2;
    }
    if (hackOptions.colourDeletePattern == 0) {
        sprintf(lines[4], "Colour delete pattern:  INWARD");
    } else if (hackOptions.colourDeletePattern == 1) {
        sprintf(lines[4], "Colour delete pattern: OUTWARD");
    } else if (hackOptions.colourDeletePattern == 2) {
        sprintf(lines[4], "Colour delete pattern:    NONE");
    }

    if (hackOptions.colourDeleteHealRate > 6) {
        hackOptions.colourDeleteHealRate = 0;
    }
    if (hackOptions.colourDeleteHealRate < 0) {
        hackOptions.colourDeleteHealRate = 6;
    }
    if (hackOptions.colourDeleteHealRate == 0) {
        sprintf(lines[5], "Colour heal:       TIMED, EASY");
    } else if (hackOptions.colourDeleteHealRate == 1) {
        sprintf(lines[5], "Colour heal:     TIMED, MEDIUM");
    } else if (hackOptions.colourDeleteHealRate == 2) {
        sprintf(lines[5], "Colour heal:       TIMED, HARD");
    } else if (hackOptions.colourDeleteHealRate == 3) {
        sprintf(lines[5], "Colour heal: 1 COLOUR PER RING");
    } else if (hackOptions.colourDeleteHealRate == 4) {
        sprintf(lines[5], "Colour heal: 5 COLOURS PER RING");
    } else if (hackOptions.colourDeleteHealRate == 5) {
        sprintf(lines[5], "Colour heal: 10 COLOURS PER RING");
    } else if (hackOptions.colourDeleteHealRate == 6) {
        sprintf(lines[5], "Colour heal:               OFF");
    }

    if (secondaryHackOptions.colourDeleteAffectsAudio > 1) {
        secondaryHackOptions.colourDeleteAffectsAudio = 0;
    }
    if (secondaryHackOptions.colourDeleteAffectsAudio < 0) {
        secondaryHackOptions.colourDeleteAffectsAudio = 1;
    }
    if (secondaryHackOptions.colourDeleteAffectsAudio == 0) {
        sprintf(lines[6], "Lost colour affects sound: OFF");
    } else if (secondaryHackOptions.colourDeleteAffectsAudio == 1) {
        sprintf(lines[6], "Lost colour affects sound:  ON");
    }

    if (secondaryHackOptions.screenSnapOnGetRing > 1) {
        secondaryHackOptions.screenSnapOnGetRing = 0;
    }
    if (secondaryHackOptions.screenSnapOnGetRing < 0) {
        secondaryHackOptions.screenSnapOnGetRing = 1;
    }
    if (secondaryHackOptions.screenSnapOnGetRing == 0) {
        sprintf(lines[7], "Screen wobble on event:    OFF");
    } else if (secondaryHackOptions.screenSnapOnGetRing == 1) {
        sprintf(lines[7], "Screen wobble on event:     ON");
    }

    linesWithBreakAfter[7] = 1;
    sprintf(lines[8], "back >");

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

        if (linesWithBreakAfter[i] != 0) {
            yPos += 8;
        }
    }
}

void showNetworkingOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Networking and Twitch", 5);

    int lineCount = 15;
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
        sprintf(lines[0], "Network/Twitch play:           OFF");
    } else {
        sprintf(lines[0], "Network/Twitch play:           ON");
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
        blockedLines[10] = 1;
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

    if (networkOptions.sendRemoveColour > 2) {
        networkOptions.sendRemoveColour = 0;
    }
    if (networkOptions.sendRemoveColour < 0) {
        networkOptions.sendRemoveColour = 2;
    }
    if (networkOptions.sendRemoveColour == 0) {
        sprintf(lines[10], "  Remove oppt colour:      OFF");
    } else if (networkOptions.sendRemoveColour == 1) {
        sprintf(lines[10], "  Remove oppt colour:    A BIT");
    } else if (networkOptions.sendRemoveColour == 2) {
        sprintf(lines[10], "  Remove oppt colour:    A LOT");
    }

    sprintf(lines[11], "");
    if (networkOptions.awaitingOpponentSettingsState == 0) {
        sprintf(lines[12], "Get opponent's settings");
    } else if (networkOptions.awaitingOpponentSettingsState == 1) {
        sprintf(lines[12], "Get opponent's settings (getting)");
    } else if (networkOptions.awaitingOpponentSettingsState == 2) {
        sprintf(lines[12], "Get opponent's settings (done!)");
    } 

    sprintf(lines[13], "");
    sprintf(lines[14], "back >");

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

void showRamEditingOptionsMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "RAM Editing", 5);

    int lineCount = 5;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }


    if (ramEditingOptionsIndex < 0) {
        ramEditingOptionsIndex = lineCount - 1;
    }
    if (ramEditingOptionsIndex >= lineCount) {
        ramEditingOptionsIndex = 0;
    }

    if (ramEditingLocationIndex > 3) {
        ramEditingLocationIndex = 3;
    }
    if (ramEditingLocationIndex < 0) {
        ramEditingLocationIndex = 0;
    }
    
    if (secondaryHackOptions.ramWritesPerRing > 4) {
        secondaryHackOptions.ramWritesPerRing = 0;
    }
    if (secondaryHackOptions.ramWritesPerRing < 0) {
        secondaryHackOptions.ramWritesPerRing = 4;
    }
    if (secondaryHackOptions.ramWritesPerRing == 0) {
        sprintf(lines[0], "Write random to ram on ring:  OFF");
    } else if (secondaryHackOptions.ramWritesPerRing == 1) {
        sprintf(lines[0], "Write random to ram on ring:   1x");
    } else if (secondaryHackOptions.ramWritesPerRing == 2) {
        sprintf(lines[0], "Write random to ram on ring:   5x");
    } else if (secondaryHackOptions.ramWritesPerRing == 3) {
        sprintf(lines[0], "Write random to ram on ring:  25x");
    } else if (secondaryHackOptions.ramWritesPerRing == 4) {
        sprintf(lines[0], "Write random to ram on ring: 100x");
    }

    char startValuesText[4][0x10];
    for (int i = 0; i < 4; i++) {
        if (secondaryHackOptions.ramWriteStartLoc[i] < 0) {
            secondaryHackOptions.ramWriteStartLoc[i] = 0xF;
        }
        if (secondaryHackOptions.ramWriteStartLoc[i] > 0xF) {
            secondaryHackOptions.ramWriteStartLoc[i] = 0;
        }

        if (ramEditingOptionsIndex == 1 && ramEditingLocationIndex == i) {
            sprintf(startValuesText[i], "<%X>", secondaryHackOptions.ramWriteStartLoc[i]);
        } else {
            sprintf(startValuesText[i], " %X ", secondaryHackOptions.ramWriteStartLoc[i]);
        }
    }
    sprintf(lines[1], "START: %s%s%s%s", startValuesText[0], startValuesText[1], startValuesText[2], startValuesText[3]);

    char endValuesText[4][0x10];
    for (int i = 0; i < 4; i++) {
        if (secondaryHackOptions.ramWriteEndLoc[i] < 0) {
            secondaryHackOptions.ramWriteEndLoc[i] = 0xF;
        }
        if (secondaryHackOptions.ramWriteEndLoc[i] > 0xF) {
            secondaryHackOptions.ramWriteEndLoc[i] = 0;
        }

        if (ramEditingOptionsIndex == 2 && ramEditingLocationIndex == i) {
            sprintf(endValuesText[i], "<%X>", secondaryHackOptions.ramWriteEndLoc[i]);
        } else {
            sprintf(endValuesText[i], " %X ", secondaryHackOptions.ramWriteEndLoc[i]);
        }
    }
    sprintf(lines[2], "END:   %s%s%s%s", endValuesText[0], endValuesText[1], endValuesText[2], endValuesText[3]);

    linesWithBreakAfter[2] = 1;

    if (secondaryHackOptions.shouldSaveRewindStates > 1) {
        secondaryHackOptions.shouldSaveRewindStates = 0;
    }
    if (secondaryHackOptions.shouldSaveRewindStates < 0) {
        secondaryHackOptions.shouldSaveRewindStates = 1;
    }
    if (secondaryHackOptions.shouldSaveRewindStates == 0) {
        sprintf(lines[3], "Allow game rewind:         OFF");
    } else {
        sprintf(lines[3], "Allow game rewind:          ON");
    }

    linesWithBreakAfter[3] = 1;
    sprintf(lines[4], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == ramEditingOptionsIndex) {
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

char* getCurrentBossRushTriggerSummary() {
    if (bossRushOptions.switchTriggers.bossHit == 0 && bossRushOptions.switchTriggers.ring == 0 && bossRushOptions.switchTriggers.land == 0 && bossRushOptions.switchTriggers.networkBossHit == 0) {
        return "switch game on: (WIN ONLY) >";
    }
    if (bossRushOptions.switchTriggers.bossHit == 1 && bossRushOptions.switchTriggers.ring == 0 && bossRushOptions.switchTriggers.land == 0 && bossRushOptions.switchTriggers.networkBossHit == 0) {
        return "switch game on:   BOSS HIT >";
    }
    if (bossRushOptions.switchTriggers.bossHit == 0 && bossRushOptions.switchTriggers.ring == 1 && bossRushOptions.switchTriggers.land == 0 && bossRushOptions.switchTriggers.networkBossHit == 0) {
        return "switch game on:   GET RING >";
    }
    if (bossRushOptions.switchTriggers.bossHit == 0 && bossRushOptions.switchTriggers.ring == 0 && bossRushOptions.switchTriggers.land == 1 && bossRushOptions.switchTriggers.networkBossHit == 0) {
        return "switch game on:       LAND >";
    }
    if (bossRushOptions.switchTriggers.bossHit == 0 && bossRushOptions.switchTriggers.ring == 0 && bossRushOptions.switchTriggers.land == 0 && bossRushOptions.switchTriggers.networkBossHit == 1) {
        return "switch game on:    NETOWRK >";
    }
    if (bossRushOptions.switchTriggers.bossHit == 1 && bossRushOptions.switchTriggers.ring == 1 && bossRushOptions.switchTriggers.land == 1 && bossRushOptions.switchTriggers.networkBossHit == 1) {
        return "switch game on:      (ALL) >";
    }

    return "switch game on:         (MANY) >";
}

void showBossRushMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Boss Rush", 5);

    int lineCount = 14;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }

    if (bossRushItemIndex < 0) {
        bossRushItemIndex = lineCount - 1;
    }
    if (bossRushItemIndex >= lineCount) {
        bossRushItemIndex = 0;
    }

    if (awaitingBossRushStart() == 1) {
        sprintf(lines[0], "boss rush:                ON");
    } else {
        sprintf(lines[0], "boss rush:               OFF");
        blockedLines[1] = 1;
        blockedLines[2] = 1;
        blockedLines[3] = 1;
        blockedLines[4] = 1;
        blockedLines[5] = 1;
        blockedLines[6] = 1;
        blockedLines[7] = 1;
        blockedLines[8] = 1;
        blockedLines[9] = 1;
        blockedLines[10] = 1;
        blockedLines[11] = 1;
    }

    // reset boss rush
    if (getShouldShowBossRushAsReadyToReset() == 1) {
        sprintf(lines[1], "start new boss rush:     YES");
    } else {
        sprintf(lines[1], "start new boss rush:      NO");
    }
    linesWithBreakAfter[1] = 1;

    // switch trigger
    sprintf(lines[2], getCurrentBossRushTriggerSummary());

    // boss order
    if (bossRushOptions.bossOrder < 0) {
        bossRushOptions.bossOrder = 4;
    }
    if (bossRushOptions.bossOrder > 4) {
        bossRushOptions.bossOrder = 0;
    }
    if (bossRushOptions.bossOrder == 0) {
        sprintf(lines[3], "boss order:           random");
    } else if (bossRushOptions.bossOrder == 1) {
        sprintf(lines[3], "boss order:     finales last");
    } else if (bossRushOptions.bossOrder == 2) {
        sprintf(lines[3], "boss order:    chronological");
    } else if (bossRushOptions.bossOrder == 3) {
        sprintf(lines[3], "boss order:  chrono per game");
    } else if (bossRushOptions.bossOrder == 4) {
        sprintf(lines[3], "boss order:  balanced chrono");
    }

    // sprintf(lines[4], "boss count: %i  %i", bossRushOptions.totalBossesIdx, getMaxSimultaneousBosses());
    if (getMaxSimultaneousBosses() > 0x70) {
        sprintf(lines[4], "boss count:        unlimited");
    } else if (getMaxSimultaneousBosses() < 10){
        sprintf(lines[4], "boss count:                %i", getMaxSimultaneousBosses());
    } else {
        sprintf(lines[4], "boss count:               %i", getMaxSimultaneousBosses());
    }

    // rings?
    if (bossRushOptions.ringsOff < 0) {
        bossRushOptions.ringsOff = 1;
    }
    if (bossRushOptions.ringsOff > 1) {
        bossRushOptions.ringsOff = 0;
    }
    if (bossRushOptions.ringsOff == 0) {
        sprintf(lines[5], "No-rings mode:           off");
    } else {
        sprintf(lines[5], "No-rings mode:            on");
    }
    linesWithBreakAfter[5] = 1;

    if (bossRushOptions.carryRingsAcrossGames < 0) {
        bossRushOptions.carryRingsAcrossGames = 1;
    }
    if (bossRushOptions.carryRingsAcrossGames > 1) {
        bossRushOptions.carryRingsAcrossGames = 0;
    }
    if (bossRushOptions.carryRingsAcrossGames == 0) {
        sprintf(lines[6], "Preserve ring count:     off");
    } else {
        sprintf(lines[6], "Preserve ring count:      on");
    }
    
    if (bossRushOptions.preventCarryInDoomsday < 0) {
        bossRushOptions.preventCarryInDoomsday = 1;
    }
    if (bossRushOptions.preventCarryInDoomsday > 1) {
        bossRushOptions.preventCarryInDoomsday = 0;
    }
    if (bossRushOptions.preventCarryInDoomsday == 0) {
        sprintf(lines[7], "   Doomsday is separate:  no");
    } else {
        sprintf(lines[7], "   Doomsday is separate: yes");
    }
    linesWithBreakAfter[7] = 1;

    if (bossRushOptions.seedEditingLocationIndex < 0) {
        bossRushOptions.seedEditingLocationIndex = 0;
    }
    if (bossRushOptions.seedEditingLocationIndex > 3) {
        bossRushOptions.seedEditingLocationIndex = 3;
    }

    if (bossRushOptions.shouldRevealSeed == 0) {
        if (bossRushItemIndex == 8) {
            sprintf(lines[8], "ORDER SEED: push c to reveal");
        } else {
            sprintf(lines[8], "ORDER SEED:           hidden");
        }
    } else {
        char seedValuesText[4][0x10];
        for (int i = 0; i < 4; i++) {
            if (bossRushOptions.orderSeed[i] < 0) {
                bossRushOptions.orderSeed[i] = 0xF;
            }
            if (bossRushOptions.orderSeed[i] > 0xF) {
                bossRushOptions.orderSeed[i] = 0;
            }

            if (bossRushItemIndex == 8 && bossRushOptions.seedEditingLocationIndex == i) {
                sprintf(seedValuesText[i], "<%X>", bossRushOptions.orderSeed[i]);
            } else {
                sprintf(seedValuesText[i], " %X ", bossRushOptions.orderSeed[i]);
            }
        }
        sprintf(lines[8], "ORDER SEED:      %s%s%s%s", seedValuesText[0], seedValuesText[1], seedValuesText[2], seedValuesText[3]);
    }
    
    sprintf(lines[9], "shuffle seed");
    linesWithBreakAfter[9] = 1;

    if (bossRushOptions.showProgress < 0) {
        bossRushOptions.showProgress = 1;
    }
    if (bossRushOptions.showProgress > 1) {
        bossRushOptions.showProgress = 0;
    }
    if (bossRushOptions.showProgress == 0) {
        sprintf(lines[10], "Show progress meter:      no");
    } else {
        sprintf(lines[10], "Show progress meter:     yes");
    }

    if (bossRushOptions.shouldExposeTrackerData < 0) {
        bossRushOptions.shouldExposeTrackerData = 1;
    }
    if (bossRushOptions.shouldExposeTrackerData > 1) {
        bossRushOptions.shouldExposeTrackerData = 0;
    }
    if (bossRushOptions.shouldExposeTrackerData == 0) {
        sprintf(lines[11], "Expose data to tracker:   no");
    } else {
        sprintf(lines[11], "Expose data to tracker:  yes");
    }

    if (bossRushOptions.shouldUseExternalMusic < 0) {
        bossRushOptions.shouldUseExternalMusic = 1;
    }
    if (bossRushOptions.shouldUseExternalMusic > 1) {
        bossRushOptions.shouldUseExternalMusic = 0;
    }
    if (bossRushOptions.shouldUseExternalMusic == 0) {
        sprintf(lines[12], "External music player:    no");
    } else {
        sprintf(lines[12], "External music player:   yes");
    }


    sprintf(lines[13], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == bossRushItemIndex) {
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

    char elapsedText[0x80];
    if (getShouldShowBossRushAsReadyToReset()) {
        sprintf(elapsedText, "Elapsed: --:--:--");
    } else {
        sprintf(elapsedText, "Elapsed: %02i:%02i:%02i", getBossRushElapsedHours(), getBossRushElapsedMins(), getBossRushElapsedSecs());
    }

    layerRenderer_writeWord256WithBorder(0, 16, yPos, elapsedText, 5, 1, 0);

    showVersionNumber();
}

int getMaxSimultaneousBosses() {

    if (bossRushOptions.totalBossesIdx < 0) {
        bossRushOptions.totalBossesIdx = 7;
    }
    if (bossRushOptions.totalBossesIdx > 7) {
        bossRushOptions.totalBossesIdx = 0;
    }

    if (bossRushOptions.totalBossesIdx == 0) {
        return 2;
    }

    if (bossRushOptions.totalBossesIdx == 1) {
        return 3;
    }

    if (bossRushOptions.totalBossesIdx == 2) {
        return 4;
    }

    if (bossRushOptions.totalBossesIdx == 3) {
        return 6;
    }

    if (bossRushOptions.totalBossesIdx == 4) {
        return 8;
    }

    if (bossRushOptions.totalBossesIdx == 5) {
        return 12;
    }

    if (bossRushOptions.totalBossesIdx == 6) {
        return 16;
    }

    if (bossRushOptions.totalBossesIdx == 7) {
        return 0x80;
    }

    return 4;
}

void flagNewSavestateLoaded() {
    saveStateWasLoaded = 1;
}

void menuDisplay_onUpdate() {
    if (shouldRerollBossRushRandomTime > 0) {
        bossRushOptions.shouldRevealSeed = 1;
        shouldRerollBossRushRandomTime --;
        bossRushOptions.orderSeed[0] = rand() % 0x10;
        bossRushOptions.orderSeed[1] = rand() % 0x10;
        bossRushOptions.orderSeed[2] = rand() % 0x10;
        bossRushOptions.orderSeed[3] = rand() % 0x10;

        bossRushOptions.didEditSeed = 0;

        if (shouldRerollBossRushRandomTime <= 3) {
            bossRushOptions.shouldRevealSeed = 0;
        }
        showBossRushMenu();
    }

    if (shouldRerollNinesChallengeRandomTime > 0) {
        ninesChallengeOptions.shouldRevealSeed = 1;
        shouldRerollNinesChallengeRandomTime --;
        shuffleNineChallengeOrderSeed();

        ninesChallengeOptions.didEditSeed = 0;

        if (shouldRerollNinesChallengeRandomTime <= 3) {
            ninesChallengeOptions.shouldRevealSeed = 0;
        }
        showNinesChallengeMenu();
    }
}

void shuffleNineChallengeOrderSeed() {
    ninesChallengeOptions.orderSeed[0] = rand() % 0x10;
    ninesChallengeOptions.orderSeed[1] = rand() % 0x10;
    ninesChallengeOptions.orderSeed[2] = rand() % 0x10;
    ninesChallengeOptions.orderSeed[3] = rand() % 0x10;

    ninesChallengeOptions.sentSeedToOpponent = 0;
    ninesChallengeOptions.receivedSeedFromOpponent = 0;
}

void showTerminalMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Alistair's Magic Box", 5);

    int lineCount = 15;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }

    if (terminalLocationIndex < 2) {
        terminalLocationIndex = lineCount - 1;
    }
    if (terminalLocationIndex > lineCount - 1) {
        terminalLocationIndex = 2;
    }

    sprintf(lines[0], "Add a gameplay effect when");
    sprintf(lines[1], "you get a ring");
    linesWithBreakAfter[1] = 1;

    sprintf(lines[2], "  Switch game");
    sprintf(lines[3], "  Switch game and keep Video Ram");
    sprintf(lines[4], "  Sonic gets faster");
    sprintf(lines[5], "  Level is corrupted");
    sprintf(lines[6], "  Memory is corrupted");
    sprintf(lines[7], "  Colours get removed");
    linesWithBreakAfter[7] = 1;

    sprintf(lines[8], "Choose a new way to play");
    linesWithBreakAfter[8] = 1;
    sprintf(lines[9], "  Sonic Boss Rush");
    sprintf(lines[10], "  4 players 1 controller");
    sprintf(lines[11], "  Sort colours");
    sprintf(lines[12], "  No background");
    sprintf(lines[13], "  No sprites");

    sprintf(lines[14], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == terminalLocationIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "%s", lines[i]);
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

void showTerminalShufflerSelectMenu() {
       layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "What games do you want to shuffle?", 5);

    int lineCount = 9;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }

    if (gameSuiteSelectIndex < 0) {
        gameSuiteSelectIndex = lineCount - 1;
    }
    if (gameSuiteSelectIndex > lineCount - 1) {
        gameSuiteSelectIndex = 1;
    }

    sprintf(lines[0], "  Sonic Classics (Mega Drive)");
    sprintf(lines[1], "  Sonic Classics (Master System)");
    sprintf(lines[2], "  All Sonic Games (Mega Drive)");
    linesWithBreakAfter[2] = 1;

    sprintf(lines[3], "  Puyo Puyo");
    sprintf(lines[4], "  Micro Machines");
    sprintf(lines[5], "  Streets of Rage");
    sprintf(lines[6], "  Shinobi");
    sprintf(lines[7], "  Four Random Games");
    linesWithBreakAfter[7] = 1;
    sprintf(lines[8], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == gameSuiteSelectIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "%s", lines[i]);
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

void showTerminalGameListMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "Choose a game", 5);

    gameCountThisTerminal = 0;
    for (int i = 0; i < 16; i++) {
        if (allowedGamesThisTerminal[i] < 0) {
            break;
        }
        gameCountThisTerminal++;
    }
    int lineCount = gameCountThisTerminal + 1;

    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }

    if (gameSuiteSelectIndex < 0) {
        gameSuiteSelectIndex = lineCount - 1;
    }
    if (gameSuiteSelectIndex > lineCount - 1) {
        gameSuiteSelectIndex = 1;
    }

    for (int i = 0; i < gameCountThisTerminal; i++) {
        sprintf(lines[i], "  %s", getTerminalNameForRom(allowedGamesThisTerminal[i]));
        linesWithBreakAfter[i] = spacesUnderGamesThisTerminal[i];
    }
    linesWithBreakAfter[gameCountThisTerminal - 1] = 1;
    sprintf(lines[lineCount - 1], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == gameSuiteSelectIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "%s", lines[i]);
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

void showNinesChallengeMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "999 Challenge", 5);

    int lineCount = 12;
    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }

    if (ninesChallengeItemIndex < 0) {
        ninesChallengeItemIndex = lineCount - 1;
    }
    if (ninesChallengeItemIndex >= lineCount) {
        ninesChallengeItemIndex = 0;
    }

    if (awaitingNinesChallengeStart() == 1) {
        sprintf(lines[0], "%i Challenge:            ON", getNinesChallengeTarget());
    } else {
        sprintf(lines[0], "%i Challenge:           OFF", getNinesChallengeTarget());
        blockedLines[1] = 1;
        blockedLines[2] = 1;
        blockedLines[3] = 1;
        blockedLines[4] = 1;
        blockedLines[5] = 1;
    }

    // reset 999 challenge
    if (getShouldShowNinesChallengeAsReadyToReset() == 1) {
        sprintf(lines[1], "start new challenge:     YES");
    } else {
        sprintf(lines[1], "start new challenge:      NO");
    }
    linesWithBreakAfter[1] = 1;

    if (ninesChallengeOptions.shouldUseAllGames < 0) {
        ninesChallengeOptions.shouldUseAllGames = 1;
    }
    if (ninesChallengeOptions.shouldUseAllGames > 1) {
        ninesChallengeOptions.shouldUseAllGames = 0;
    }
    if (ninesChallengeOptions.shouldUseAllGames == 0) {
        sprintf(lines[2], "Game selection:     One Game");
    } else {
        sprintf(lines[2], "Game selection:   Multi-Game");
    }

    if (ninesChallengeOptions.shouldUseRandomOrder < 0) {
        ninesChallengeOptions.shouldUseRandomOrder = 1;
    }
    if (ninesChallengeOptions.shouldUseRandomOrder > 1) {
        ninesChallengeOptions.shouldUseRandomOrder = 0;
    }
    if (ninesChallengeOptions.shouldUseRandomOrder == 0) {
        sprintf(lines[3], "Level order:          Normal");
        blockedLines[4] = 1;
        blockedLines[5] = 1;
    } else {
        sprintf(lines[3], "Level order:          Random");
    }
    linesWithBreakAfter[3] = 1;

    if (ninesChallengeOptions.shouldRevealSeed == 0) {
        if (ninesChallengeItemIndex == 4) {
            sprintf(lines[4], "ORDER SEED: push c to reveal");
        } else {
            if (ninesChallengeOptions.receivedSeedFromOpponent) {
                sprintf(lines[4], "ORDER SEED:   got opponent's");
            } else if (ninesChallengeOptions.sentSeedToOpponent) {
                sprintf(lines[4], "ORDER SEED: sent to opponent");
            } else {
                sprintf(lines[4], "ORDER SEED:           hidden");
            }
        }
    } else {
        char seedValuesText[4][0x10];
        for (int i = 0; i < 4; i++) {
            if (ninesChallengeOptions.orderSeed[i] < 0) {
                ninesChallengeOptions.orderSeed[i] = 0xF;
            }
            if (ninesChallengeOptions.orderSeed[i] > 0xF) {
                ninesChallengeOptions.orderSeed[i] = 0;
            }

            if (ninesChallengeItemIndex == 4 && ninesChallengeOptions.seedEditingLocationIndex == i) {
                sprintf(seedValuesText[i], "<%X>", ninesChallengeOptions.orderSeed[i]);
            } else {
                sprintf(seedValuesText[i], " %X ", ninesChallengeOptions.orderSeed[i]);
            }
        }
        sprintf(lines[4], "ORDER SEED:      %s%s%s%s", seedValuesText[0], seedValuesText[1], seedValuesText[2], seedValuesText[3]);
    }
    
    sprintf(lines[5], "shuffle seed");
    linesWithBreakAfter[5] = 1;

    if (ninesChallengeOptions.allowTacticalDeaths < 0) {
        ninesChallengeOptions.allowTacticalDeaths = 1;
    }
    if (ninesChallengeOptions.allowTacticalDeaths > 1) {
        ninesChallengeOptions.allowTacticalDeaths = 0;
    }
    if (ninesChallengeOptions.allowTacticalDeaths == 0) {
        sprintf(lines[6], "Use deaths as warp:       NO");
    } else {
        sprintf(lines[6], "Use deaths as warp:      YES");
    }

    if (ninesChallengeOptions.targetTotalIndex < 0) {
        ninesChallengeOptions.targetTotalIndex = 1;
    }
    if (ninesChallengeOptions.targetTotalIndex > 1) {
        ninesChallengeOptions.targetTotalIndex = 0;
    }
    sprintf(lines[7], "Target total:      %i rings", getNinesChallengeTarget());

    if (ninesChallengeOptions.shouldUseCheckpoints < 0) {
        ninesChallengeOptions.shouldUseCheckpoints = 2;
    }
    if (ninesChallengeOptions.shouldUseCheckpoints > 2) {
        ninesChallengeOptions.shouldUseCheckpoints = 0;
    }
    if (ninesChallengeOptions.shouldUseCheckpoints == 0) {
        sprintf(lines[8], "Ring bank:                OFF");
    } else if (ninesChallengeOptions.shouldUseCheckpoints == 1) {
        sprintf(lines[8], "Ring bank:    ON, CAN DEPLETE");
    } else {
        sprintf(lines[8], "Ring bank:    ON,   PERMANENT");
    }
    linesWithBreakAfter[8] = 1;


    if (ninesChallengeOptions.useOnlineRace < 0) {
        ninesChallengeOptions.useOnlineRace = 1;
    }
    if (ninesChallengeOptions.useOnlineRace > 1) {
        ninesChallengeOptions.useOnlineRace = 0;
    }
    if (ninesChallengeOptions.useOnlineRace == 0) {
        sprintf(lines[9], "Networked race:           NO");
    } else {
        sprintf(lines[9], "Networked race:          YES");
    }


    sprintf(lines[10], "Request opponent's seed");
    linesWithBreakAfter[10] = 1;


    sprintf(lines[11], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == ninesChallengeItemIndex) {
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

    char elapsedText[0x80];
    if (getShouldShowNinesChallengeAsReadyToReset()) {
        sprintf(elapsedText, "Elapsed: --:--:--");
    } else {
        sprintf(elapsedText, "Elapsed: %02i:%02i:%02i", getNinesChallengeElapsedHours(), getNinesChallengeElapsedMins(), getNinesChallengeElapsedSecs());
    }

    layerRenderer_writeWord256WithBorder(0, 16, yPos + 8, elapsedText, 5, 1, 0);

    showVersionNumber();
}

void showBossRushTriggerSelectMenu() {
    layerRenderer_clearLayer(0);

    layerRenderer_fill(0, 8, 8, DEFAULT_WIDTH - 16, DEFAULT_HEIGHT - 16, 0xFF);
    layerRenderer_writeWord256Centred(0, DEFAULT_WIDTH / 2, 16, "WHEN DO YOU WANT TO SWITCH?", 5);

    int lineCount = 5;

    char lines[lineCount][0x80];
    int blockedLines[lineCount];
    int linesWithBreakAfter[lineCount];
    for (int i = 0; i < lineCount; i++) {
        blockedLines[i] = 0;
        linesWithBreakAfter[i] = 0;
    }

    if (bossRushTriggerSelectItemIndex < 0) {
        bossRushTriggerSelectItemIndex = lineCount - 1;
    }
    if (bossRushTriggerSelectItemIndex > lineCount - 1) {
        bossRushTriggerSelectItemIndex = 0;
    }

    if (bossRushOptions.switchTriggers.bossHit < 0) {
        bossRushOptions.switchTriggers.bossHit = 1;
    }
    if (bossRushOptions.switchTriggers.bossHit > 1) {
        bossRushOptions.switchTriggers.bossHit = 0;
    }
    if (bossRushOptions.switchTriggers.bossHit == 0) {
        sprintf(lines[0], "Switch on damage boss:    no");
    } else {
        sprintf(lines[0], "Switch on damage boss:   yes");
    }
    linesWithBreakAfter[0] = 1;

    if (bossRushOptions.switchTriggers.ring < 0) {
        bossRushOptions.switchTriggers.ring = 1;
    }
    if (bossRushOptions.switchTriggers.ring > 1) {
        bossRushOptions.switchTriggers.ring = 0;
    }
    if (bossRushOptions.switchTriggers.ring == 0) {
        sprintf(lines[1], "Switch on get ring:       no");
    } else {
        sprintf(lines[1], "Switch on get ring:      yes");
    }
    linesWithBreakAfter[1] = 1;
    
    if (bossRushOptions.switchTriggers.land < 0) {
        bossRushOptions.switchTriggers.land = 1;
    }
    if (bossRushOptions.switchTriggers.land > 1) {
        bossRushOptions.switchTriggers.land = 0;
    }
    if (bossRushOptions.switchTriggers.land == 0) {
        sprintf(lines[2], "Switch on touch ground:   no");
    } else {
        sprintf(lines[2], "Switch on touch ground:  yes");
    }
    linesWithBreakAfter[2] = 1;
    
    if (bossRushOptions.switchTriggers.networkBossHit < 0) {
        bossRushOptions.switchTriggers.networkBossHit = 1;
    }
    if (bossRushOptions.switchTriggers.networkBossHit > 1) {
        bossRushOptions.switchTriggers.networkBossHit = 0;
    }
    if (bossRushOptions.switchTriggers.networkBossHit == 0) {
        sprintf(lines[3], "On Networked boss hit     no");
    } else {
        sprintf(lines[3], "On Networked boss hit    yes");
    }

    sprintf(lines[lineCount - 1], "back >");

    int yPos = 32;
    for (int i = 0; i < lineCount; i++) {
        if (cartLoader_string32AreEqual(lines[i], "back >") == 1) {
            yPos += 8;
        }

        char toPrint[0x100];
        if (i == bossRushTriggerSelectItemIndex) {
            sprintf(toPrint, ">> %s", lines[i]);
        } else {
            sprintf(toPrint, "%s", lines[i]);
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

void applyNinesChallengeSeedFromOpponent(int rawSeed) {
    for (int i = 0; i < 4; i++) {
        int divisor = 100 * 100 * 100;
        for (int j = 0; j < i; j++) {
            divisor /= 100;
        }
        ninesChallengeOptions.orderSeed[i] = (rawSeed / divisor) % 100;
    }            
    ninesChallengeOptions.receivedSeedFromOpponent = 1;  

    if (activeMenu == MENU_LISTING_NINES_CHALLENGE) {
        showNinesChallengeMenu();
    }
}

int getNinesChallengeTarget() {
    if (ninesChallengeOptions.targetTotalIndex == 1) {
        return 9999;
    }
    return 999;
}