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

extern void cartLoader_run();
extern void cartLoader_appendToLog(char *text);
extern int cartLoader_getActiveCartIndex();
extern AAGameListing cartLoader_getActiveGameListing();
extern AAGameTransferListing cartLoader_getActiveGameTransferListing();
extern AAScoreMonitorListing cartLoader_getActiveScoreMonitorListing();
extern void cartLoader_loadRandomRom();
extern unsigned int cartLoader_getRomCount();
extern void cartLoader_getRomFileName(int index, char intoArray[]);
extern void cartLoader_getRomFilePrefix(int index, char intoArray[]);
extern void writeFolderPathIntoArray32(char array32[]);

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

#endif