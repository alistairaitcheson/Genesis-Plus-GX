#ifndef _AA_CART_LOADER_H_
#define _AA_CART_LOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "AACommonTypes.h"

extern void cartLoader_run();
extern void cartLoader_appendToLog(char *text);
extern int cartLoader_getActiveCartIndex();
extern AAGameListing cartLoader_getActiveGameListing();
extern void cartLoader_loadRandomRom();
extern unsigned int cartLoader_getRomCount();
extern void cartLoader_getRomFileName(int index, char intoArray[]);
extern void writeFolderPathIntoArray32(char array32[]);

extern void cartLoader_loadAllSaveStatesFromDisk();
extern void cartLoader_saveAllSaveStatesToDisk();
extern void cartLoader_applyHackOptions();
extern void cartLoader_loadSaveStateForCurrentGame();

void listFiles(const char *path);
void addRomListing(char *path);
int pathIsRom(char *path, int pathLen);
void cartLoader_loadRomAtIndex(int index, int shouldCache);
void concatenate_string(char *original, char *add);
void writeStringToArray32(char *source, char dest[]);
void copyGameListing(int fromGame, int toGame);
extern void saveSaveStateForCurrentGame();
int pathIsSaveState(char *path, int pathLen);
void initialiseDirectory();

#endif