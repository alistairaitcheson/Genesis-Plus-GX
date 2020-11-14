#include "AACartLoader.h"

#include "shared.h" // <--- this needs to be included at the top of every file, for compiler reasons I don't understand
#include <stdio.h>
#include <sys/types.h>
#include "include/dirent.h"
#include "vdp_render.h"
#include "AAModConsole.h"
#include "AACommonTypes.h"
#include <sys/stat.h>
#include "AALayerRenderer.h"
#include "genesis.h"
#include "AAMenuDisplay.h"

#define MAX_ROMS 0x100

static unsigned int romCount;
static char *folderPath = "_magicbox";
static char *folderPathWithTail = "_magicbox/";
static char romFileNames[MAX_ROMS][0x100];
static int romsRemovedFromRandomiser[MAX_ROMS];

static char *logLines[0x100];
static unsigned int logLineCount;

static AAGameListing gameListings[0x100];
static int gameListingCount = 0;

static char romHeaderBuffer[0x20];

static char *completeLog = "";

static FILE *globalLogWriter;
static int openedLogWriter = 0;
static int lastLoadedIndex = -1;

static char loadedRomName[0x100];
static int hasLoadedRom = 0;

static int initialisedDirectory = 0;

static uint8 cachedSaveStates[MAX_ROMS][STATE_SIZE];
static uint8 hasCachedSaveState[MAX_ROMS];

void writeFolderPathIntoArray32(char array32[]) {
    writeStringToArray32(folderPath, array32);
}

void cartLoader_run() {
    for (int i = 0; i < MAX_ROMS; i++) {
        hasCachedSaveState[i] = 0;
        romsRemovedFromRandomiser[i] = 0;
    }
    
    cartLoader_appendToLog("cartLoader_run");

    initialiseDirectory();

    cartLoader_appendToLog("will list files");

    listFiles(folderPathWithTail);

    cartLoader_appendToLog("Listed files");

    writeStringToArray32("NONE", gameListings[0].gameId);
    gameListings[0].ringByte = 0;
    gameListings[0].specialRingByte = 0;
    gameListings[0].updateHUDFlags[0] = 0;
    gameListings[0].livesBytes[0] = 0;
    gameListings[0].livesByteDestinations[0] = 0;
    gameListings[0].timeBytes[0] = 0;
    gameListings[0].timeByteDestinations[0] = 0;
    gameListings[0].panicBytes[0] = 0;
    gameListings[0].panicByteDestinations[0] = 0;
    gameListings[0].extraLifeFlags[0] = 0;

    writeStringToArray32("SONICTHEHEDGEHOG", gameListings[1].gameId);// = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','\0'};
    gameListings[1].ringByte = 0xFE20;
    gameListings[1].specialRingByte = 0;    
    gameListings[1].updateHUDFlags[0] = 0xFE1C;
    gameListings[1].updateHUDFlags[1] = 0xFE1D;
    gameListings[1].updateHUDFlags[2] = 0xFE1E;
    gameListings[1].updateHUDFlags[3] = 0xFE1F;
    gameListings[1].extraLifeFlags[0] = 0xFE1B;
    gameListings[1].extraLifeFlags[1] = 0xFE54;
    gameListings[1].extraLifeFlags[2] = 0;
    gameListings[1].livesBytes[0] = 0xFE12;
    gameListings[1].livesByteDestinations[0] = 0x5; 
    gameListings[1].livesBytes[1] = 0xFE13;
    gameListings[1].livesByteDestinations[1] = 0x5; 
    gameListings[1].livesBytes[2] = 0xFE1C;
    gameListings[1].livesByteDestinations[2] = 0xFF; 
    gameListings[1].timeBytes[0] = 0xFE22;
    gameListings[1].timeByteDestinations[0] = 1;
    gameListings[1].timeBytes[1] = 0xFE23;
    gameListings[1].timeByteDestinations[1] = 1;
    gameListings[1].panicBytes[0] = 0xF72E;
    gameListings[1].panicByteDestinations[0] = 0;
    gameListings[1].panicBytes[1] = 0xF72F;
    gameListings[1].panicByteDestinations[1] = 0;
    gameListings[1].panicBytes[2] = 0xF75C;
    gameListings[1].panicByteDestinations[2] = 0;

    writeStringToArray32("SONICTHEHEDGEHOG2", gameListings[2].gameId);//gameListings[1].gameId = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','2','\0'};
    copyGameListing(1, 2);
    gameListings[2].panicBytes[0] = 0xB00C;
    gameListings[2].panicByteDestinations[0] = 0x55;
    gameListings[2].panicBytes[1] = 0xB00D;
    gameListings[2].panicByteDestinations[1] = 0x55;

    writeStringToArray32("SONICTHEHEDGEHOG3", gameListings[3].gameId);//gameListings[2].gameId = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','3','\0'};
    copyGameListing(1, 3);
    gameListings[3].specialRingByte = 0xE43A;
    gameListings[3].panicBytes[0] = 0xB014;
    gameListings[3].panicByteDestinations[0] = 0x55;
    gameListings[3].panicBytes[1] = 0xB015;
    gameListings[3].panicByteDestinations[1] = 0x55;
    gameListings[3].extraLifeFlags[0] = 0xFE1B;
    gameListings[3].extraLifeFlags[1] = 0xFE43;
    gameListings[3].extraLifeFlags[2] = 0xFE61;

    writeStringToArray32("SONIC&KNUCKLES", gameListings[4].gameId);//gameListings[3].gameId = {'S','O','N','I','C','&','K','N','U','C','K','L','E','S','\0'};
    copyGameListing(3, 4);

    writeStringToArray32("SONIC3&KNUCKLES", gameListings[5].gameId);
    copyGameListing(3, 5);

    gameListingCount = 6;

    cartLoader_appendToLog("finished cartLoader_run");
}

void writeStringToArray32(char *source, char dest[]) {
    for (int i = 0; i < 0x20; i++) {
        dest[i] = 0;
    }

    for (int i = 0; i < 0x20; i++) {
        dest[i] = source[i];
        if (source[i] == 0 || source[i] == '\0') {
            return;
        }
    }
}

/**
 * Lists all files and sub-directories at given path.
 */
void listFiles(const char *path)
{
    struct dirent *dp;
    DIR *dir = opendir(path);

    while ((dp = readdir(dir)) != NULL)
    {
        cartLoader_appendToLog(dp->d_name);

        // if (pathIsSaveState(dp->d_name, dp->d_namlen)) {
        //     char pathToDelete[0x100];
        //     sprintf(pathToDelete, "%s%s", path, dp->d_name);
        //     cartLoader_appendToLog("removing save state");
        //     cartLoader_appendToLog(pathToDelete);
        //     remove(pathToDelete);
        // }
        
        if (pathIsRom(dp->d_name, dp->d_namlen) != 0 && romCount < MAX_ROMS) {
            char name[0x100];
            for (int i = 0; i < 0x100; i++) {
                if (i < dp->d_namlen) {
                    name[i] = dp->d_name[i];
                } else if (i == dp->d_namlen) {
                    name[i] = '\0';
                } else {
                    name[i] = 0;
                }
            }

            // because I can't figure out strings and pointers
            for (int i = 0; i < 0x100; i++) {
                romFileNames[romCount][i] = name[i]; 
            }
            romCount++;
            cartLoader_appendToLog("new-added rom");
            cartLoader_appendToLog(romFileNames[romCount]);
        }
    }

    closedir(dir);
}

int pathIsSaveState(char *path, int pathLen) {
    if (pathLen > 10) {
        if (path[pathLen - 10] == '.' && 
            path[pathLen - 9] == 's' && 
            path[pathLen - 8] == 'a' && 
            path[pathLen - 7] == 'v' && 
            path[pathLen - 6] == 'e' && 
            path[pathLen - 5] == 's' && 
            path[pathLen - 4] == 't' && 
            path[pathLen - 3] == 'a' &&
            path[pathLen - 2] == 't' &&
            path[pathLen - 1] == 'e' ) {
            return 1;
        }

    }
    return 0;
}

unsigned int cartLoader_getRomCount() {
    return romCount;
}

static int loadAttemptCount = 0;
void cartLoader_loadRandomRom() {
    int candidates[MAX_ROMS];
    int candidateCount = 0;
    for (int i = 0; i < romCount; i++) {
        if (romsRemovedFromRandomiser[i] == 0 && i != lastLoadedIndex) {
            candidates[candidateCount] = i;
            candidateCount++;
        }
    }

    loadAttemptCount++;
    if (candidateCount > 0) {
        int whichCandidate = rand() % candidateCount;
        int nextIndex = candidates[whichCandidate]; 
        
        cartLoader_loadRomAtIndex(nextIndex, 1);
        loadAttemptCount = 0;
    }
}

int cartLoder_getLastLoadedIndex() {
    return lastLoadedIndex;
}

void cartLoader_removeCurrentGameFromRandomiser() {
    if (lastLoadedIndex >= 0 && lastLoadedIndex < MAX_ROMS) {
        romsRemovedFromRandomiser[lastLoadedIndex] = 1;
        cartLoader_loadRandomRom();
    }
}

int cartLoader_gameIsBlockedFromRandomiser(int index) {
    return romsRemovedFromRandomiser[index];
}

void cartLoader_toggleGameBlockedAtIndex(int index) {
    if (index >= 0 && index < MAX_ROMS) {
        romsRemovedFromRandomiser[index] = 1 - romsRemovedFromRandomiser[index];
    }
}

void cartLoader_getRomFileName(int index, char intoArray[]) {
    for(int i = 0; i < 0x100; i++) {
        intoArray[i] = romFileNames[index][i];
    }
}

void cartLoader_loadRomAtIndex(int index, int shouldCache) {
    if (index >= romCount) {
        char logStr[0x100];
        sprintf(logStr, "Could not load rom and index %d (%d roms loaded)", index, romCount);
        cartLoader_appendToLog(logStr);
        return;
    }

    if (hasLoadedRom != 0 && shouldCache != 0) {
        saveSaveStateForCurrentGame();
        cacheDataToCarryOver();
    }

    char vramCache[0x10000];
    if (shouldCache != 0 && menuDisplay_getHackOptions().copyVram > 0) {
        for (int i = 0; i < 0x10000; i++) {
            vramCache[i] = aa_genesis_getVRamValue(i);
        }
    }

    // cartLoader_appendToLog("building rom path to:");
    // cartLoader_appendToLog(romFileNames[index]);

    char fullPath[0x100];
    int pathIndex = 0;
    for (int i = 0; i < 0x100; i++) {
        if (folderPath[i] == '\0') {
            break;
        } else {
            fullPath[i] = folderPath[i];
            pathIndex++;
        }
    }
    fullPath[pathIndex] = '/';
    pathIndex++;

    for (int i = 0; i < 0x100; i++) {
        if (romFileNames[index][i] == '\0') {
            break;
        } else {
            fullPath[pathIndex] = romFileNames[index][i];
            pathIndex++;
        }
    }
    fullPath[pathIndex] = '\0';

    cartLoader_appendToLog(fullPath);

    load_rom(fullPath);
    system_init();
    system_reset();
    lastLoadedIndex = index;

    sprintf(loadedRomName, "%s", romFileNames[index]);
    hasLoadedRom = 1;

    cartLoader_loadSaveStateForCurrentGame();

    if (hasLoadedRom != 0 && shouldCache != 0) {
        restoreCarriedOverData();
    }

    aa_genesis_updateLastRam();

    if (shouldCache != 0 && menuDisplay_getHackOptions().copyVram > 0) {
        int probality = 100;
        if (menuDisplay_getHackOptions().copyVram == 2) {
            probality = 50;
        } 
        if (menuDisplay_getHackOptions().copyVram == 3) {
            probality = 10;
        } 
        if (menuDisplay_getHackOptions().copyVram == 4) {
            probality = 1;
        } 

        for (int i = 0; i < 0x10000; i++) {
            if (rand() % 100 < probality) {
                aa_genesis_setVRamValue(i, vramCache[i]);
            }
        }
    }
}

int cartLoader_getActiveCartIndex() {
    modConsole_getRomHeader(romHeaderBuffer);

    // cartLoader_appendToLog("cartLoader_getActiveCartIndex");
    // cartLoader_appendToLog(romHeaderBuffer);

    for (int i = 1; i < gameListingCount; i++) {
        if (modconsole_array32sAreEqual(romHeaderBuffer, gameListings[i].gameId)) {
            return i;
        }
    }
    return 0;
}

AAGameListing cartLoader_getActiveGameListing() {
    return gameListings[cartLoader_getActiveCartIndex()];
}

// void addRomListing(char *path) {
//     cartLoader_appendToLog("addRomListing");
//     cartLoader_appendToLog(path);
//     romFileNames[romCount] = path;
//     romCount++;

//     char str[0x100];
//     sprintf(str, "%d", romCount);
//     cartLoader_appendToLog(str);
// }

int pathIsRom(char *path, int pathLen) {
    if (pathLen > 4) {
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 's' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 'd' ) {
            return 1;
        }
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 'b' && path[pathLen - 2] == 'i' && path[pathLen - 1] == 'n' ) {
            return 1;
        }
        if (path[pathLen -3] == '.' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 'd') {
            return 1;
        }
    }
    return 0;
}

void initialiseDirectory() {
    if (initialisedDirectory == 0) {
        char command[0x100];
        sprintf(command, "mkdir %s", folderPath);
        system(command);
        initialisedDirectory = 1;
    }
}

void cartLoader_appendToLog(char *text) {
    initialiseDirectory();
    // return; // <----------------- replace this with something togglable!

    if (openedLogWriter == 0) {
        openedLogWriter = 1;
        char path[0x100];
        sprintf(path, "%s/__log.txt", folderPath);
        remove(path);
        globalLogWriter = fopen(path, "w");
    }

    fprintf(globalLogWriter, text);
    fprintf(globalLogWriter, "\n");

}

void concatenate_string(char *original, char *add)
{
   while(*original)
      original++;
     
   while(*add)
   {
      *original = *add;
      add++;
      original++;
   }
   *original = '\0';
}

void copyGameListing(int fromGame, int toGame) {
    // cartLoader_appendToLog("copyGameListing");
    // cartLoader_appendToLog(gameListings[fromGame].gameId);
    // cartLoader_appendToLog("TO");
    // cartLoader_appendToLog(gameListings[toGame].gameId);
    // cartLoader_appendToLog("");

    gameListings[toGame].ringByte = gameListings[fromGame].ringByte;
    gameListings[toGame].specialRingByte = gameListings[fromGame].specialRingByte;

    for (int i = 0; i < 0x20; i++) {
        gameListings[toGame].livesBytes[i] = gameListings[fromGame].livesBytes[i];
        gameListings[toGame].livesByteDestinations[i] = gameListings[fromGame].livesByteDestinations[i];
        gameListings[toGame].timeBytes[i] = gameListings[fromGame].timeBytes[i];
        gameListings[toGame].timeByteDestinations[i] = gameListings[fromGame].timeByteDestinations[i];
        gameListings[toGame].panicBytes[i] = gameListings[fromGame].panicBytes[i];
        gameListings[toGame].panicByteDestinations[i] = gameListings[fromGame].panicByteDestinations[i];
        gameListings[toGame].updateHUDFlags[i] = gameListings[fromGame].updateHUDFlags[i];
        gameListings[toGame].extraLifeFlags[i] = gameListings[fromGame].extraLifeFlags[i];
    }
}

void saveSaveStateForCurrentGame() {
    // char tempLog[256];
    // sprintf(tempLog,"Caching save state %d (%s)", lastLoadedIndex, loadedRomName);
    // cartLoader_appendToLog(tempLog);

    state_save(cachedSaveStates[lastLoadedIndex]);
    hasCachedSaveState[lastLoadedIndex] = 1;
}

void cartLoader_loadSaveStateForCurrentGame() {
    if (hasCachedSaveState[lastLoadedIndex] == 0) {
        return;
    }

    // char tempLog[256];
    // sprintf(tempLog,"Loading save state %d (%s) %d", lastLoadedIndex, loadedRomName, hasCachedSaveState[lastLoadedIndex]);
    // cartLoader_appendToLog(tempLog);

    state_load(cachedSaveStates[lastLoadedIndex]);
}

void cartLoader_saveAllSaveStatesToDisk() {
    cartLoader_appendToLog("cartLoader_saveAllSaveStatesToDisk");

    for (int i = 0; i < romCount; i++) {
        if (hasCachedSaveState[i] != 0) {
            char path[256];
            sprintf(path, "%s_%s.savestate", folderPathWithTail, romFileNames[i]);
            
            char tempLog[256];
            sprintf(tempLog,"Loading save state %d", i);
            cartLoader_appendToLog(tempLog);
            cartLoader_appendToLog(path);

            FILE *f = fopen(path,"wb");
            if (f)
            {
                fwrite(&cachedSaveStates[i], STATE_SIZE, 1, f);
                fclose(f);
                cartLoader_appendToLog("success!");
            } else {
                cartLoader_appendToLog("no state found");
            }
        } else {
            char tempLog[256];
            sprintf(tempLog,"No cached state at index %d", i);
            cartLoader_appendToLog(tempLog);
        }
        cartLoader_appendToLog(" -- ");
    }
}

void cartLoader_loadAllSaveStatesFromDisk() {
    for (int i = 0; i < romCount; i++) {
        char path[256];
        sprintf(path, "%s_%s.savestate", folderPathWithTail, romFileNames[i]);

        char tempLog[256];
        sprintf(tempLog,"Saving save state %d", i);
        cartLoader_appendToLog(tempLog);
        cartLoader_appendToLog(path);
        
        FILE *f = fopen(path,"rb");
        if (f)
        {
            fread(&cachedSaveStates[i], STATE_SIZE, 1, f);
            fclose(f);
            cartLoader_appendToLog("success!");
            hasCachedSaveState[i] = 1;
        } else {
            cartLoader_appendToLog("not found - could not load");
        }
        cartLoader_appendToLog(" -- ");
    }
}

void cartLoader_applyHackOptions(int gameHasStarted) {
    if (menuDisplay_getHackOptions().loadFromSavedState && gameHasStarted == 0) {
        cartLoader_loadAllSaveStatesFromDisk();
    }
}

static PersistValuesData cachedPersistValues;

void cacheDataToCarryOver() {
    PersistValuesOptions options = menuDisplay_getPersistValuesOptions();
    AAGameListing gameListing = cartLoader_getActiveGameListing();

    // we abuse the fact that the first two values are "life count" and the third is "update the life counter plz"
    cachedPersistValues.lives[0] = -1;
    cachedPersistValues.lives[1] = -1;
    if (options.lives != 0) {
        if (gameListing.livesBytes[0] > 0) {
            unsigned char value = aa_genesis_getWorkRam(gameListing.livesBytes[0]);
            if (value > 0 && value < 100) {
                cachedPersistValues.lives[0] = value;

                // char logMsg[0x100];
                // sprintf(logMsg, "Caching lives (0) %d from byte %04X", cachedPersistValues.lives[0], gameListing.livesBytes[0]);
                // cartLoader_appendToLog(logMsg);
            }
        }
        if (gameListing.livesBytes[1] > 0) {
            unsigned char value = aa_genesis_getWorkRam(gameListing.livesBytes[1]);

            if (value > 0 && value < 100) {
                cachedPersistValues.lives[1] = value;

                // char logMsg[0x100];
                // sprintf(logMsg, "Caching lives (1) %d from byte %04X", cachedPersistValues.lives[1], gameListing.livesBytes[1]);
                // cartLoader_appendToLog(logMsg);
            }
        }
    }

    cachedPersistValues.rings[0] = -1;
    cachedPersistValues.rings[1] = -1;
    if (options.rings != 0) {
        if (gameListing.ringByte > 0) {
            cachedPersistValues.rings[0] = aa_genesis_getWorkRam(gameListing.ringByte);
        }
    }

    cachedPersistValues.extraLifeFlag = -1;
    
    if (options.rings != 0 && options.lives != 0) {
        // char logMsg2[0x100];
        // sprintf(logMsg2, "Checking to cache extra lives state");
        // cartLoader_appendToLog(logMsg2);

        if (gameListing.ringByte > 0) {
            int ringCount = aa_genesis_getWorkRam(gameListing.ringByte);
            // char logMsg3[0x100];
            // sprintf(logMsg3, "Ring byte is valid. You have %d rings", ringCount);
            // cartLoader_appendToLog(logMsg3);

            if (ringCount > 100) {
                cachedPersistValues.extraLifeFlag = 0b11111111;
                // char logMsg4[0x100];
                // sprintf(logMsg4, "Over 100 rings. Setting flag to %d", cachedPersistValues.extraLifeFlag);
                // cartLoader_appendToLog(logMsg4);
            } else {
                cachedPersistValues.extraLifeFlag = 0;
                // char logMsg4[0x100];
                // sprintf(logMsg4, "Less than 100 rings. Setting flag to %d", cachedPersistValues.extraLifeFlag);
                // cartLoader_appendToLog(logMsg4); 
            }
            // if (ringCount > 200) {
            //     cachedPersistValues.extraLifeFlag = 0b11111111;
            //     char logMsg4[0x100];
            //     sprintf(logMsg4, "Over 200 rings. Setting flag to %d", cachedPersistValues.extraLifeFlag);
            //     cartLoader_appendToLog(logMsg4);
            // }
        }
    }
}

void restoreCarriedOverData() {
    PersistValuesOptions options = menuDisplay_getPersistValuesOptions();
    AAGameListing gameListing = cartLoader_getActiveGameListing();

    // cartLoader_appendToLog(".. restoreCarriedOverData");
    // cartLoader_appendToLog(gameListing.gameId);
    for (int i = 0; i < 3; i++) {
        // char logMsgA[0x100];
        // sprintf(logMsgA, "Extra life state at %04X is currently %02X", gameListing.extraLifeFlags[i], aa_genesis_getWorkRam(gameListing.extraLifeFlags[i]));
        // cartLoader_appendToLog(logMsgA);
    }

    if (cachedPersistValues.lives[0] != -1) {
        aa_genesis_setWorkRam(gameListing.livesBytes[0], cachedPersistValues.lives[0] % 0x100);

        // char logMsg[0x100];
        // sprintf(logMsg, "Applying lives (0) %d to byte %04X", cachedPersistValues.lives[0], gameListing.livesBytes[0]);
        // cartLoader_appendToLog(logMsg);
    }
    if (cachedPersistValues.lives[1] != -1) {
        aa_genesis_setWorkRam(gameListing.livesBytes[1], cachedPersistValues.lives[1] % 0x100);
        
        // char logMsg[0x100];
        // sprintf(logMsg, "Applying lives (1) %d to byte %04X", cachedPersistValues.lives[1], gameListing.livesBytes[1]);
        // cartLoader_appendToLog(logMsg);
    }
    
    if (cachedPersistValues.rings[0] != -1) {
        aa_genesis_setWorkRam(gameListing.ringByte, cachedPersistValues.rings[0] % 0x100);
    }

    // char logMsg2[0x100];
    // sprintf(logMsg2, "Cached extra lives state is %d", cachedPersistValues.extraLifeFlag);
    // cartLoader_appendToLog(logMsg2);
    if (cachedPersistValues.extraLifeFlag != -1) {
        for (int i = 0; i < 3; i++) {
            if (gameListing.extraLifeFlags[i] > 0) {
                aa_genesis_setWorkRam(gameListing.extraLifeFlags[i], cachedPersistValues.extraLifeFlag % 0x100);
                // char logMsg[0x100];
                // sprintf(logMsg, "Applying cached extra life state %d to byte (%d) %04X (now %02X)", cachedPersistValues.extraLifeFlag, i, gameListing.extraLifeFlags[i], aa_genesis_getWorkRam(gameListing.extraLifeFlags[i]));
                // cartLoader_appendToLog(logMsg);
            }
        }
    }

    for (int i = 0; i < 0x20; i++) {
        if (gameListing.updateHUDFlags[i] >= 0) {
            aa_genesis_setWorkRam(gameListing.updateHUDFlags[i] % 0x10000, 0xFF);
        }
    }
}