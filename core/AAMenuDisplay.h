#ifndef _AA_MENU_DISPLAY_H_
#define _AA_MENU_DISPLAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "types.h"

#define MENU_LISTING_NONE 0
#define MENU_LISTING_TITLE 1
#define MENU_LISTING_CHOOSE_GAME 2
#define MENU_LISTING_SETTINGS 3
#define MENU_LISTING_IN_GAME 4
#define MENU_LISTING_RANDOMISED_ROMS 5
#define MENU_LISTING_PERSIST_VALUES 6
#define MENU_LISTING_RAM_DETECTIVE 7
#define MENU_LISTING_GAME_SWAP_OPITONS 8
#define MENU_LISTING_QUALITY_OF_LIFE 9
#define MENU_LISTING_SAVE_STATE_OPTIONS 10
#define MENU_LISTING_SONIC_SPECIFIC_OPTIONS 11
#define MENU_LISTING_VISUALS_OPTIONS 12
#define MENU_LISTING_PIXEL_DETECTIVE 13
#define MENU_LISTING_NETWORKING 14
#define MENU_LISTING_RAM_EDITING 15
#define MENU_LISTING_TERMINAL 16
#define MENU_LISTING_BOSS_RUSH 17
#define MENU_LISTING_TERMINAL_SHUFFLER 19
#define MENU_LISTING_TERMINAL_GAME_LIST 20
#define MENU_LISTING_NINES_CHALLENGE 22

#define TERMINAL_RULSET_SHUFFLER 1
#define TERMINAL_RULSET_SHUFFLER_WITH_VRAM 2
#define TERMINAL_RULSET_RINGS_MAKE_FASTER 3
#define TERMINAL_RULSET_RINGS_CORRUPT_LEVEL 4
#define TERMINAL_RULSET_RINGS_CORRUPT_RAM 5
#define TERMINAL_RULSET_REMOVE_COLOUR 6
#define TERMINAL_RULSET_SORT_COLOURS 7
#define TERMINAL_RULSET_BOSS_RUSH 8
#define TERMINAL_RULSET_CONTROLLER 9
#define TERMINAL_RULSET_NO_SPRITES_ALT 11
#define TERMINAL_RULSET_NO_BACKGROUNDS_ALT 12

typedef struct {
    int infiniteLives;
    int infiniteTime;
    int copyVram;
    int switchGameType;
    int cooldownOnSwitch;
    int speedUpOnRing;
    int loadFromSavedState;
    int automaticallySaveStatesFreq;
    int shouldWriteToLog;
    int shouldSortColours;
    int limitedColourType;
    int shouldHideLayers;
    int shouldShowSwapCount;
    int shouldShowDeathCount;
    int overwriteLevelType; // 0 = off,  1 = with random number, 2 = with 0
    int overwriteLevelDifficulty; // 0 = easy (5x), 1 = medium (10x), 2 = hard (20x), 3 = extreme (50x)
    int swapOrder; // 0 = random, 1 = alphabetical
    int randomiseVelocityOnRing;
    int colourDeleteTrigger; // 0 = off, 1 = on ring, 2 = 10 seconds, 3 = 60 seconds, 
    int colourDeletePattern; // 0 = black hole, 1 = collapse inwards, 2 = none
    int colourDeleteHealRate; // 0 = easy, 1 = medium, 2 = hard, 3 = off
} HackOptions;

typedef struct {
    int colourDeleteAffectsAudio; // 0 = off, 1 = on
    int screenSnapOnGetRing; // 0 = off, 1 = on
    int ramWritesPerRing; // 0 = off, 1 = 1x, 2 = 5x, 3 = 25x, 4 = 100x
    int ramWriteStartLoc[4];
    int ramWriteEndLoc[4];
    int shouldSaveRewindStates;
    int vramWritesPerRing; // 0 = off, 1 = 1x, 2 = 5x, 3 = 25x, 4 = 100x
} SecondaryHackOptions;

typedef struct {
    int lives;
    int rings;
    int topSpeed;
    int momentum;
    int time;
    int score;
} PersistValuesOptions;

typedef struct {
    int startLoc[4];
    int startValueIndex;
    int endLoc[4];
    int endValueIndex;
    int seekValue[2];
    int seekValueIndex;
    int minFrames;
    int shouldShow;
    int trackerLocations[8][4];
    int trackerValueIndexes[8];
    int shouldShowTracker;
} RamDetectiveOptions;

typedef struct {
    int coordsListings[8][4];
    int coordsListingIndex;
    int shouldShow;
} PixelDetectiveOptions;

typedef struct {
    int networkingIsActive;
    int sendSwitchGame;
    int sendSpeedUp;
    int sendWriteIntoLevelDifficulty;
    int sendRandomiseVelocity;
    int allowSoloEffectswhenNetworked;
    int awaitingOpponentSettingsState;
    int sendRemoveColour;
} NetworkOptions;

typedef struct {
    int switchTrigger;
    int bossOrder;
    int totalBossesIdx; // 0 = 4, 1 = 6, 2 = 8, 3 = 12, 4 = all
    int ringsOff;
    int carryRingsAcrossGames;
    int preventCarryInDoomsday;
    int orderSeed[4];
    int seedEditingLocationIndex;
    int shouldRevealSeed;

    int showProgress;
    int shouldExposeTrackerData;
    int shouldUseExternalMusic;
} BossRushOptions;

typedef struct {
    int shouldUseAllGames;
    int shouldUseRandomOrder;
    int showProgress;

    int orderSeed[4];
    int seedEditingLocationIndex;
    int shouldRevealSeed;
} NinesChallengeOptions;

extern void menuDisplay_showMenu(int menuNum);
extern void menuDisplay_hideMenu();
extern int menuDisplay_onButtonPress(int buttonIndex);
extern HackOptions menuDisplay_getHackOptions();
extern void menuDisplay_initialise();
extern int menuDisplay_isShowing();
extern void menuDisplay_hideMenuUnlessQueued();
extern PersistValuesOptions menuDisplay_getPersistValuesOptions();
extern void menuDisplay_renderRamDetective();
extern void menuDisplay_updateRamDetective();
extern void menuDisplay_logRamStateToTrackedValues();
extern void menuDisplay_renderPixelDetective();
extern void menuDisplay_updatePixelDetective(int line, uint8 linebuf[2][0x200]);
extern NetworkOptions menuDisplay_getNetworkOptions();
extern int menuDisplay_areSoloEffectsAllowed();
extern void menuDisplay_applyNetworkOptionSwitch(char command, int asPositive);
extern void menuDisplay_sendNetworkOptionsToOpponent();
extern SecondaryHackOptions menuDisplay_getSecondaryHackOptions();
void applySecondaryHacksFromArray256(int array256[]);
void applySecondaryHacksDefaultValues();
extern BossRushOptions menuDisplay_getBossRushOptions();
void applyDefaultBossRushValues();

extern void menuDisplay_toggleVisibleLayers();
extern void menuDisplay_showAllVisibleLayers();

void beginGame();
void showTitleMenu();
void refreshMenu();
void showChooseGameMenu();
void incrementOption(int byAmount);
void showOptionsMenu();
void showRandomisedGameMenu();
void showPersistValuesMenu();
void togglePersistValue(int index);

void applyPersistValuesFromArray256(int array256[]);
void applyDefaultPersistValues();

void applyDefaultRamDetectiveValues();
void applyNetworkOptionsDefaultValues();
void applyNetworkOptionsFromArray256(int array256[]);

void applySettingsFromArray256(int array256[]);
void applyDefaultSettings();
void saveHackOptions();

void activateInGameMenuItem();
void showInGameOptionsMenu();
void showRamDetectiveMenu();
void ramDetectivePressFaceButton(int direction);
void ramDetectivePressDPadDir(int direction);
void pixelDetectivePressFaceButton(int direction);
void pixelDetectivePressDPadDir(int direction);

void chooseMainMenuOption();

void showGameSwapOptionsMenu();
void showQualityOfLifeOptionsMenu();
void showSaveStateOptionsMenu();
void showSonicSpecificOptionsMenu();
void showVisualsOptionsMenu();
void showPixelDetectiveMenu();
void showNetworkingOptionsMenu();
void showRamEditingOptionsMenu();
void showBossRushMenu();

void incrementGameSwapOption(int direction);
void incrementQualityOfLifeOption(int direction);
void incrementSaveStateOption(int direction);
void incrementSonicSpecificOption(int direction);
void incrementVisualsOption(int direction);
void incrementNetworkOption(int direction);
void incrementBossRushOption(int direction, int buttonIndex);

void clearLogRamState();

extern void menudisplay_applyToggleVRAMState(int vramState);

extern int menuDisplay_shouldRamEditingOptionsShowAsOn();
void incrementRamEditingOptionWithDPad(int direction);
void incrementRamEditingOptionWithFaceButton(int direction);

extern void menuDisplay_applyPresetRules(int rulesIndex);
extern void menuDisplay_showTerminalMenu();
void showOriginalTerminalMenu();
void showTerminalMenu();

int getMaxSimultaneousBosses();
void flagNewSavestateLoaded();
void menuDisplay_onUpdate();
void initialiseChosenTerminalGame();
void applyAllowedGamesForCurrentTerminalSelection();
void clearAllowedGamesThisTerminal();
void showTerminalShufflerSelectMenu();
void showTerminalGameListMenu();
void enterTerminalOption();
void chooseGameSuite();
extern void menuDisplay_beginIdleMode();
extern void menuDisplay_generateRulesNameForCurrentGame();
extern char* menuDisplay_getCurrentRulesName();
void showVersionNumber();
void switchToRandomAllowedGame();
int terminalRulesAreActive();

void showNinesChallengeMenu();
void incrementNinesChallengeOption(int direction, int buttonIndex);
NinesChallengeOptions menuDisplay_getNinesChallengeOptions() ;

#endif