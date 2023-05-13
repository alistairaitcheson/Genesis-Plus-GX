#ifndef _AA_CART_LOADER_H_
#define _AA_CART_LOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "AACommonTypes.h"

#define CART_TYPE_MEGADRIVE 0
#define CART_TYPE_MASTERSYSTEM 1
#define CART_TYPE_GAMEGEAR 2
#define CART_TYPE_SEGACD 3


typedef struct {
    int lives[2];
    int rings[2];
    int topSpeed[4];
    int momentum[8];
    int time[4];
    int score[4];
} PersistValuesData;

extern void initialiseBossRush();
extern void cartLoader_run();
extern void cartLoader_appendToLog(char *text);
extern int cartLoader_getActiveCartIndex();
extern AAGameListing cartLoader_getActiveGameListing();
extern AAStandTriggerListing cartLoader_getActiveStandTriggerListing();
extern AAGameTransferListing cartLoader_getActiveGameTransferListing();
extern AAScoreMonitorListing cartLoader_getActiveScoreMonitorListing();
extern AALevelEditListing cartLoader_getActiveLevelEditListing();
extern MomentumControlListing cartLoader_getMomentumControlListing();
extern void cartLoader_loadRandomRom();
extern unsigned int cartLoader_getRomCount();
extern void cartLoader_getRomFileName(int index, char intoArray[]);
extern void cartLoader_getRomFilePrefix(int index, char intoArray[]);
extern void writeFolderPathIntoArray32(char array32[]);
extern void writeShortenedFileName(char source256[], char output256[], int length);

extern int cartLoader_base10Array32ToInt(char array32[]);
extern int cartLoader_base10CharToInt(char character);

extern void cartLoader_loadAllSaveStatesFromDisk();
extern void cartLoader_saveAllSaveStatesToDisk();
extern void cartLoader_applyHackOptions(int gameHasStarted);
extern void cartLoader_loadSaveStateForCurrentGame();
extern void cartLoader_removeCurrentGameFromRandomiser();
extern int cartLoader_gameIsBlockedFromRandomiser(int index);
extern int cartLoder_getLastLoadedIndex();
extern void cartLoader_toggleGameBlockedAtIndex(int index);
extern void cartLoader_restoreCarriedOverData();

extern int cartLoader_string32AreEqual(char strA[], char strB[]);
extern int cartLoader_getSwapCount();

void listFiles(const char *path, char prefix[]);
void addRomListing(char *path);
int pathIsRom(char *path, int pathLen);
void cartLoader_loadRomAtIndex(int index, int shouldCache);
void concatenate_string(char *original, char *add);
void writeStringToArray32(char *source, char dest[]);
void copyGameListing(int fromGame, int toGame);
extern void saveSaveStateForCurrentGame();
int pathIsSaveState(char *path, int pathLen);
void initialiseDirectory();

void cacheDataToCarryOver();
void flagHUDtoUpdate();

void cartLoader_cacheSaveStateBeforeMenu();
void cartLoader_loadSaveStateForQuitMenu();
extern int cartLoader_consoleForCurrentCart();
extern int cartLoader_getFoundZipCount();
int pathIsZip(char *path, int pathLen);
int fileName256IsCD(char fileName[]);

void zeroAllListings();

extern void cartLoader_updatePixelTracker(int line, unsigned char linebuf[2][0x200]);
extern void cartLoader_checkPixelTrackerForStateChange();

extern void cartloader_initialiseNetworkDirectories();
extern void cartloader_initialiseRewindDirectory();
extern void cartLoader_checkNetworkForActions();
extern void cartLoader_writeActionToNetwork(char action256[]);

void clearSendDirectory();
void clearRecvDirectory();
void clearRewindDirectory();

extern void cartLoader_saveRewindStateForCurrentGame();
extern int cartLoader_loadRewindStateForCurrentGame();
void deleteRewindState(int gameIndex, int stateIndex);

void cartLoader_isolateRomAtIndex(int index);

extern void beginBossRush();
extern void onBossHit();
extern void onBossDefeated();
extern int getActiveBossRushIndex();
extern BossRushChallengeListing getActiveBossRushListing();
extern void bumpToNextBossRush();
extern void saveActiveBossRushSlot();
extern void loadActiveBossRushSlot();
extern void mapBossRushesToRoms();
extern void queueBossRushSlots();
extern void queueBossRushInSlot(int slot);
extern void populateBossRushes();
void addBossRushListing(int gameIndex, int zoneIndex, int actIndex, unsigned int objectLocationStart, unsigned int objectLocationEnd, unsigned int objectLocationSize, unsigned int healthByteOffset, unsigned int defeatedByte, unsigned int defeatedValue);
void duplicateBossRushListing(int listingIndex, int zoneIndex, int actIndex);
void populateMostRecentBossRush(unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3);
void applyGenerationToMostRecentBossRush(unsigned int generation);
void applyEndValuesToMostRecentBossRush(unsigned int endLoc, unsigned int endVal);
void populateBossRushObjectIds4(int listingIndex, unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3);
void populateBossRushObjectIds8(int listingIndex, unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3, unsigned int id4, unsigned int id5, unsigned int id6, unsigned int id7);
void saveStateForCurrentBoss();
void loadStateForCurrentBoss();
void populateMostRecentBossRush4(unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3);
void populateMostRecentBossRush8(unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3, unsigned int id4, unsigned int id5, unsigned int id6, unsigned int id7);

void toggleStartBossRush();
int shouldUseBossRush();
extern int checkForBossRushStart();
extern int awaitingBossRushStart();
extern void cartLoader_loadBossRushSaveStatesFromDisk();
int challengeCanBeQueued(int index);
void checkForBossRushComplete();
void applyZoneLocationValuesToMostRecentBossRush(unsigned int zone, unsigned int act, unsigned int checkpoint);
void onBossRushComplete();
int getActiveBossRushSlotId();
int getBossRushIndexInSlot(int slot);
int getBossRushComplete();

void setShouldResetBossRush(int val);
int getShouldResetBossRush();
int getHasInitialisedBossRush();
void setHasInitialisedBossRush(int val);
int getShouldShowBossRushAsReadyToReset();
void resetAllBossRushSlots();

int getCompletedRushCount();
int getEnabledRushCount();
int numberOfRushesInGame(int gameIdx);
int getCountOfQueueableRushes();
void applyBossRushCachedRings();
int getBossRushRandomSeedFromMenu();
int getBossRushSeedWithPrefix(int prefix);
void cacheRingCountInBossRush(int becauseOfHit);

void clearBossRushProgress();
int indexOfBossRushProgress(int gameId, int zoneId, int actId);
int indexOfLowestUnusedBossProgressSlot();
void incrementFrameCountOfActiveBossRush();
void flagActiveBossRushProgressAsComplete();
void flagAllBossRushProgressAsComplete();
void saveBossRushProgress();
void shuffleBossSwitchRandomNumbers();
int getNextRandomBossRushNumber();
void setStartBossRush(int toValue);

void cartLoader_setGameBlockedAtIndex(int index, int toValue);
void cartLoader_setAllGamesAsBlocked();
void cartLoader_setAllGamesAsUnblocked();
void cartLoader_unblockGamesWithCartNumber(int cartNumber);
void cartLoader_setRandomSelectionOfGamesAsUnblocked(int maxCount);
void cartLoader_clearSaveStates();
int getCartIndexForRomAtIndex(int index);
extern char* getTerminalNameForRom(int romIndex);
extern char* getNameOfTriggerForGame(int cartIndex);
extern char* cartLoader_getNameOfTriggerForActiveGame();
void abortAllBossRushSettings();
void cartLoader_blockGamesWithCartNumber(int cartNumber);
void cartLoader_loadCurrentStartupStateFromDisk();

AAMusicOverrideListing cartLoader_getActiveMusicOverrideListing();
void cartLoader_reduceCurrentHaltCountdown();
void cartLoader_beginCurrentHaltCountdown();

void abortAllNinesChallengeSettings();
int awaitingNinesChallengeStart();
int getNinesChallengeComplete();
void setShouldResetNinesChallenge(int val);
int getShouldShowNinesChallengeAsReadyToReset();
int getShouldResetNinesChallenge();
void toggleStartNinesChallenge();
int shouldUseNinesChallenge();
void beginNinesChallenge();
NinesChallengeStageListing getCurrentNinesChallengeStage();
void bumpNinesChallengeLevel();
NinesChallengeGameParameters getActiveNinesChallengeGameParameters();
void cartLoader_loadNinesChallengeSaveStatesFromDisk();

char* getBossRushRingCarryValuesForLogs();
int getBossRushRingCarryValues(int index);
int getNinesChallengeRandomSeedFromMenu();
int getBossRushRingCarryTotal();
void completeNinesChallenge();
void enforceBumpToSameNinesStageAgain();
int getBestNinesChallengeRingCount();
int checkForNinesChallengeStart();

#endif