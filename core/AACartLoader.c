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
static AAGameTransferListing gameTransferListings[0x100];
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

static uint8 saveStateBeforeMenu[STATE_SIZE];

static int gameSwapCount = 0;

static unsigned char lastSystemType;

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
    gameListings[0].accelerationType = 0;
    gameListings[0].valueWriteDuration = 0;
    gameTransferListings[0].ringBytesForTransfer[0] = 0;
    gameTransferListings[0].speedBytesForTransfer[0] = 0;
    gameTransferListings[0].timeBytesForTransfer[0] = 0;
    gameTransferListings[0].momentumBytesForTransfer[0] = 0;
    gameTransferListings[0].scoreBytesForTransfer[0] = 0;

    writeStringToArray32("SONICTHEHEDGEHOG", gameListings[1].gameId);// = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','\0'};
    gameListings[1].ringByte = 0xFE20;
    gameListings[1].specialRingByte = 0;    
    gameListings[1].updateHUDFlags[0] = 0xFE1C;
    gameListings[1].updateHUDFlags[1] = 0xFE1D;
    gameListings[1].updateHUDFlags[2] = 0xFE1E;
    gameListings[1].updateHUDFlags[3] = 0xFE1F;
    gameTransferListings[1].ringBytesForTransfer[0] = 0xFE20;
    gameTransferListings[1].ringBytesForTransfer[1] = 0xFE21;
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
    gameListings[1].accelerationType = 1;
    gameListings[1].valueWriteDuration = 0;
    gameTransferListings[1].speedBytesForTransfer[0] = 0xF760;
    gameTransferListings[1].speedBytesForTransfer[1] = 0xF761;
    gameTransferListings[1].speedBytesForTransfer[2] = 0xF762;
    gameTransferListings[1].speedBytesForTransfer[3] = 0xF763;
    gameTransferListings[1].timeBytesForTransfer[0] = 0xFE22;
    gameTransferListings[1].timeBytesForTransfer[1] = 0xFE23;
    gameTransferListings[1].timeBytesForTransfer[2] = 0xFE24;
    gameTransferListings[1].timeBytesForTransfer[3] = 0xFE25;
    gameTransferListings[1].momentumBytesForTransfer[0] = 0xD010;
    gameTransferListings[1].momentumBytesForTransfer[1] = 0xD011;
    gameTransferListings[1].momentumBytesForTransfer[2] = 0xD012;
    gameTransferListings[1].momentumBytesForTransfer[3] = 0xD013;
    gameTransferListings[1].momentumBytesForTransfer[4] = 0xD014;
    gameTransferListings[1].momentumBytesForTransfer[5] = 0xD015;
    gameTransferListings[1].momentumBytesForTransfer[6] = 0xD022;
    gameTransferListings[1].momentumBytesForTransfer[7] = 0xD03C;
    gameTransferListings[1].scoreBytesForTransfer[0] = 0xFE26;
    gameTransferListings[1].scoreBytesForTransfer[1] = 0xFE27;
    gameTransferListings[1].scoreBytesForTransfer[2] = 0xFE28;
    gameTransferListings[1].scoreBytesForTransfer[3] = 0xFE29;

    writeStringToArray32("SONICTHEHEDGEHOG2", gameListings[2].gameId);//gameListings[1].gameId = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','2','\0'};
    copyGameListing(1, 2);
    gameListings[2].panicBytes[0] = 0xB00C;
    gameListings[2].panicByteDestinations[0] = 0x55;
    gameListings[2].panicBytes[1] = 0xB00D;
    gameListings[2].panicByteDestinations[1] = 0x55;
    gameTransferListings[2].momentumBytesForTransfer[0] = 0xB010;
    gameTransferListings[2].momentumBytesForTransfer[1] = 0xB011;
    gameTransferListings[2].momentumBytesForTransfer[2] = 0xB012;
    gameTransferListings[2].momentumBytesForTransfer[3] = 0xB013;
    gameTransferListings[2].momentumBytesForTransfer[4] = 0xB014;
    gameTransferListings[2].momentumBytesForTransfer[5] = 0xB015;
    gameTransferListings[2].momentumBytesForTransfer[6] = 0xB022;
    gameTransferListings[2].momentumBytesForTransfer[7] = 0xB03C;

    writeStringToArray32("SONICTHEHEDGEHOG3", gameListings[3].gameId);//gameListings[2].gameId = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','3','\0'};
    copyGameListing(1, 3);
    gameListings[3].specialRingByte = 0xE43A;
    gameListings[3].panicBytes[0] = 0xB014;
    gameListings[3].panicByteDestinations[0] = 0x55;
    gameListings[3].panicBytes[1] = 0xB015;
    gameListings[3].panicByteDestinations[1] = 0x55;
    gameTransferListings[3].momentumBytesForTransfer[0] = 0xB018;
    gameTransferListings[3].momentumBytesForTransfer[1] = 0xB019;
    gameTransferListings[3].momentumBytesForTransfer[2] = 0xB01A;
    gameTransferListings[3].momentumBytesForTransfer[3] = 0xB01B;
    gameTransferListings[3].momentumBytesForTransfer[4] = 0xB01C;
    gameTransferListings[3].momentumBytesForTransfer[5] = 0xB01D;
    gameTransferListings[3].momentumBytesForTransfer[6] = 0xB02A;
    gameTransferListings[3].momentumBytesForTransfer[7] = 0xB040;

    writeStringToArray32("SONIC&KNUCKLES", gameListings[4].gameId);//gameListings[3].gameId = {'S','O','N','I','C','&','K','N','U','C','K','L','E','S','\0'};
    copyGameListing(3, 4);

    writeStringToArray32("SONIC3&KNUCKLES", gameListings[5].gameId);
    copyGameListing(3, 5);

    writeStringToArray32("SONIC3DBLAST", gameListings[6].gameId);
    gameListings[6].ringByte = 0x0A5A;
    gameListings[6].specialRingByte = 0xA17C;    // <-- to do!
    gameListings[6].livesBytes[0] = 0x0680;
    gameListings[6].livesByteDestinations[0] = 0x5; 
    gameListings[6].accelerationType = 2;
    gameListings[6].valueWriteDuration = 60; // once per second

    writeStringToArray32("SonicSpinball", gameListings[7].gameId);
    gameListings[7].ringByte = 0x57A1;
    gameListings[7].specialRingByte = 0;    
    gameListings[7].livesBytes[0] = 0x579F;
    gameListings[7].livesByteDestinations[0] = 0x5; 
    gameListings[7].valueWriteDuration = 20 * 60 * 60; // only update lives every 20 seconds
    
    writeStringToArray32("15900", gameListings[8].gameId); // Sonic 2 MS
    gameListings[8].ringByte = 0x1299;
    gameListings[8].specialRingByte = 0;    
    gameListings[8].livesBytes[0] = 0x1298;
    gameListings[8].livesByteDestinations[0] = 0x5; 
    gameListings[8].valueWriteDuration = 0;
    gameListings[8].timeBytes[0] = 0x12B9;
    gameListings[8].timeByteDestinations[0] = 1;
    gameListings[8].timeBytes[1] = 0x12BA;
    gameListings[8].timeByteDestinations[1] = 1;
    gameListings[8].valueWriteDuration = 60;
        
    writeStringToArray32("76700", gameListings[9].gameId); // Sonic 1 MS
    gameListings[9].ringByte = 0x12AA;
    gameListings[9].specialRingByte = 0;    
    gameListings[9].livesBytes[0] = 0x1246;
    gameListings[9].livesByteDestinations[0] = 0x5; 
    gameListings[9].timeBytes[0] = 0x12CE;
    gameListings[9].timeByteDestinations[0] = 1;
    gameListings[9].valueWriteDuration = 60;

    writeStringToArray32("21900", gameListings[10].gameId); // Sonic Chaos MS
    gameListings[10].ringByte = 0x129A;
    gameListings[10].specialRingByte = 0;    
    gameListings[10].livesBytes[0] = 0x1299;
    gameListings[10].livesByteDestinations[0] = 0x5; 
    gameListings[10].timeBytes[0] = 0x12C0;
    gameListings[10].timeByteDestinations[0] = 1;
    gameListings[10].valueWriteDuration = 0;
    gameListings[10].valueWriteDuration = 60;

    gameListingCount = 11;
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

        gameSwapCount++;
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
        cartLoader_restoreCarriedOverData();
        modConsole_flagToApplyCache();
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

    cartLoader_appendToLog("*** Loaded game ***");
    cartLoader_appendToLog(cartLoader_getActiveGameListing().gameId);

    // if (lastSystemType != system_hw) {
        if (cartLoader_cartIsSMS() != 0) {
            vdp_setAlistairOffset(64, 16); /// <-- either not working yet or not being called
            // replace with a multiply effect?
        } else {
            vdp_setAlistairOffset(0, 0);
        }
        lastSystemType = system_hw;
    // }
}

int cartLoader_cartIsSMS() {
    char checkText[0x20];
    sprintf(checkText, /*"TMR SEGA"*/ "54 4D 52 20 53 45 47 41");
    char romText[0x20];
    sprintf(romText, "%02X %02X %02X %02X %02X %02X %02X %02X", 
        cart.rom[0x7FF0 + 0], 
        cart.rom[0x7FF0 + 1], 
        cart.rom[0x7FF0 + 2], 
        cart.rom[0x7FF0 + 3], 
        cart.rom[0x7FF0 + 4], 
        cart.rom[0x7FF0 + 5], 
        cart.rom[0x7FF0 + 6], 
        cart.rom[0x7FF0 + 7]);
    // for (int i = 8; i < 8; i++) {
    //     sprintf(romText, "%s%02X", romText, cart.rom[0x7FF0 + i]);
    //     // romText[i] = cart.rom[0x7FF0 + i]; // <-- I don't think this works...
    // }
    // romText[8] = '\0';

    char debugLog[0x100];
    sprintf(debugLog, "Comparing ROM HEADERS: %s > %s", checkText, romText);
    cartLoader_appendToLog(debugLog);

    if (cartLoader_string32AreEqual(checkText, romText) != 0) {
        cartLoader_appendToLog("It's a match!");
        return 1;
    }
    cartLoader_appendToLog("It's not a match...");
    return 0;
}

int cartLoader_getActiveCartIndex() {
    modConsole_getRomHeader(romHeaderBuffer);

    cartLoader_appendToLog("cartLoader_getActiveCartIndex");
    cartLoader_appendToLog(romHeaderBuffer);

    for (int i = 1; i < gameListingCount; i++) {
        if (modconsole_array32sAreEqual(romHeaderBuffer, gameListings[i].gameId)) {
            return i;
        }
    }

    // not found - check Master System headers
    modConsole_getMasterSystemProductId(romHeaderBuffer);

    cartLoader_appendToLog("! No ROM found - checking Master System product ID");
    cartLoader_appendToLog(romHeaderBuffer);    
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

AAGameTransferListing cartLoader_getActiveGameTransferListing() {
    return gameTransferListings[cartLoader_getActiveCartIndex()];
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
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 'g' && path[pathLen - 2] == 'e' && path[pathLen - 1] == 'n' ) {
            return 1;
        }
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 's' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 'd' ) {
            return 1;
        }
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 'b' && path[pathLen - 2] == 'i' && path[pathLen - 1] == 'n' ) {
            return 1;
        }
        if (path[pathLen -3] == '.' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 'd') {
            return 1;
        }
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 's' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 's' ) {
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
    if (menuDisplay_getHackOptions().shouldWriteToLog == 0) {
        return;
    }
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
    gameListings[toGame].valueWriteDuration = gameListings[fromGame].valueWriteDuration;

    for (int i = 0; i < 8; i++) {
        gameListings[toGame].livesBytes[i] = gameListings[fromGame].livesBytes[i];
        gameListings[toGame].livesByteDestinations[i] = gameListings[fromGame].livesByteDestinations[i];
        gameListings[toGame].timeBytes[i] = gameListings[fromGame].timeBytes[i];
        gameListings[toGame].timeByteDestinations[i] = gameListings[fromGame].timeByteDestinations[i];
        gameListings[toGame].panicBytes[i] = gameListings[fromGame].panicBytes[i];
        gameListings[toGame].panicByteDestinations[i] = gameListings[fromGame].panicByteDestinations[i];
        gameListings[toGame].updateHUDFlags[i] = gameListings[fromGame].updateHUDFlags[i];

        gameTransferListings[toGame].ringBytesForTransfer[i] = gameTransferListings[fromGame].ringBytesForTransfer[i];
        gameTransferListings[toGame].speedBytesForTransfer[i] = gameTransferListings[fromGame].speedBytesForTransfer[i];
        gameTransferListings[toGame].timeBytesForTransfer[i] = gameTransferListings[fromGame].timeBytesForTransfer[i];
        gameTransferListings[toGame].momentumBytesForTransfer[i] = gameTransferListings[fromGame].momentumBytesForTransfer[i];
        gameTransferListings[toGame].scoreBytesForTransfer[i] = gameTransferListings[fromGame].scoreBytesForTransfer[i];
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

void cartLoader_cacheSaveStateBeforeMenu() {
    cartLoader_appendToLog("cartLoader_cacheSaveStateBeforeMenu");
    state_save(saveStateBeforeMenu);
}

void cartLoader_loadSaveStateForQuitMenu() {
    state_load(saveStateBeforeMenu);
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
    AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();

    cartLoader_appendToLog(" - - - cacheDataToCarryOver");
    cartLoader_appendToLog(gameListing.gameId);

    // we abuse the fact that the first two values are "life count" and the third is "update the life counter plz"
    cachedPersistValues.lives[0] = -1;
    cachedPersistValues.lives[1] = -1;
    if (options.lives != 0) {
        if (gameListing.livesBytes[0] > 0) {
            unsigned char value = aa_genesis_getWorkRam(gameListing.livesBytes[0] % 0x10000);
            if (value > 0 && value < 100) {
                cachedPersistValues.lives[0] = value;
            }
        }
        if (gameListing.livesBytes[1] > 0) {
            unsigned char value = aa_genesis_getWorkRam(gameListing.livesBytes[1] % 0x10000);

            if (value > 0 && value < 100) {
                cachedPersistValues.lives[1] = value;
            }
        }
    }

    cachedPersistValues.rings[0] = -1;
    cachedPersistValues.rings[1] = -1;
    if (options.rings != 0) {
        if (gameTransferListing.ringBytesForTransfer[0] > 0) {
            cachedPersistValues.rings[0] = aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[0] % 0x10000);

            char tempLog[0x100];
            sprintf(tempLog, "Persisiting ring count %d at %04X", cachedPersistValues.rings[0], gameTransferListing.ringBytesForTransfer[0]);
            cartLoader_appendToLog(tempLog);
        }
        //  else {
        //     char tempLog[0x100];
        //     sprintf(tempLog, "Will not persisit at %04X", gameTransferListing.ringBytesForTransfer[0]);
        //     cartLoader_appendToLog(tempLog);
        // }
        if (gameTransferListing.ringBytesForTransfer[1] > 0) {
            cachedPersistValues.rings[1] = aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[1] % 0x10000);
            
            char tempLog[0x100];
            sprintf(tempLog, "Persisiting ring count %d at %04X", cachedPersistValues.rings[1], gameTransferListing.ringBytesForTransfer[1]);
            cartLoader_appendToLog(tempLog);
        }
    } else {
        cartLoader_appendToLog("Not caching ring count");
    }

    for (int i = 0; i < 4; i++) {
        cachedPersistValues.topSpeed[i] = -1;
    }
    if (options.topSpeed != 0) {
        for (int i = 0; i < 4; i++) {
            if (gameTransferListing.speedBytesForTransfer[i] > 0) {
                cachedPersistValues.topSpeed[i] = aa_genesis_getWorkRam(gameTransferListing.speedBytesForTransfer[i] % 0x10000);                
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        cachedPersistValues.time[i] = -1;
    }
    if (options.time != 0) {
        for (int i = 0; i < 4; i++) {
            if (gameTransferListing.timeBytesForTransfer[i] > 0) {
                cachedPersistValues.time[i] = aa_genesis_getWorkRam(gameTransferListing.timeBytesForTransfer[i] % 0x10000);       
                char tempLog[0x100];
                sprintf(tempLog, "Persisiting time value (%i) %d at %04X", i, cachedPersistValues.time[i], gameTransferListing.timeBytesForTransfer[i]);
                cartLoader_appendToLog(tempLog);         
            } else {
                char tempLog[0x100];
                sprintf(tempLog, "Will not persist time value (%i) at %04X", i, gameTransferListing.timeBytesForTransfer[i]);
                cartLoader_appendToLog(tempLog);   
            }
        }
    }
    
    for (int i = 0; i < 8; i++) {
        cachedPersistValues.momentum[i] = -1;
    }
    if (options.momentum != 0) {
        for (int i = 0; i < 8; i++) {
            if (gameTransferListing.momentumBytesForTransfer[i] > 0) {
                cachedPersistValues.momentum[i] = aa_genesis_getWorkRam(gameTransferListing.momentumBytesForTransfer[i] % 0x10000);       
                char tempLog[0x100];
                sprintf(tempLog, "Persisiting momentum value (%i) %d at %04X", i, cachedPersistValues.momentum[i], gameTransferListing.momentumBytesForTransfer[i]);
                cartLoader_appendToLog(tempLog);         
            } else {
                char tempLog[0x100];
                sprintf(tempLog, "Will not persist momentum value (%i) at %04X", i, gameTransferListing.momentumBytesForTransfer[i]);
                cartLoader_appendToLog(tempLog);   
            }
        }
    }
        
    for (int i = 0; i < 4; i++) {
        cachedPersistValues.score[i] = -1;
    }
    if (options.score != 0) {
        for (int i = 0; i < 4; i++) {
            if (gameTransferListing.scoreBytesForTransfer[i] > 0) {
                cachedPersistValues.score[i] = aa_genesis_getWorkRam(gameTransferListing.scoreBytesForTransfer[i] % 0x10000);       
                char tempLog[0x100];
                sprintf(tempLog, "Persisiting score value (%i) %d at %04X", i, cachedPersistValues.score[i], gameTransferListing.scoreBytesForTransfer[i]);
                cartLoader_appendToLog(tempLog);         
            } else {
                char tempLog[0x100];
                sprintf(tempLog, "Will not persist score value (%i) at %04X", i, gameTransferListing.scoreBytesForTransfer[i]);
                cartLoader_appendToLog(tempLog);   
            }
        }
    }
}

void cartLoader_restoreCarriedOverData() {
    PersistValuesOptions options = menuDisplay_getPersistValuesOptions();
    AAGameListing gameListing = cartLoader_getActiveGameListing();
    AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();

    cartLoader_appendToLog("cartLoader_restoreCarriedOverData");
    cartLoader_appendToLog(gameListing.gameId);

    if (cachedPersistValues.lives[0] != -1) {
        aa_genesis_setWorkRam(gameListing.livesBytes[0], cachedPersistValues.lives[0] % 0x100);
        flagHUDtoUpdate();
    }
    if (cachedPersistValues.lives[1] != -1) {
        aa_genesis_setWorkRam(gameListing.livesBytes[1], cachedPersistValues.lives[1] % 0x100);
        flagHUDtoUpdate();
    }
    
    if (cachedPersistValues.rings[0] != -1) {
        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[0] % 0x10000, cachedPersistValues.rings[0] % 0x100);

        char tempLog[0x100];
        sprintf(tempLog, "Wrote ring count %d to %04X", cachedPersistValues.rings[0], gameTransferListing.ringBytesForTransfer[0]);
        cartLoader_appendToLog(tempLog);
        flagHUDtoUpdate();
    }
    if (cachedPersistValues.rings[1] != -1) {
        aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[1] % 0x10000, cachedPersistValues.rings[1] % 0x100);
        flagHUDtoUpdate();
    }

    for (int i = 0; i < 4; i++) {
        if (cachedPersistValues.topSpeed[i] != -1 && gameTransferListing.speedBytesForTransfer[i] > 0) {
            aa_genesis_setWorkRam(gameTransferListing.speedBytesForTransfer[i] % 0x10000, cachedPersistValues.topSpeed[i] % 0x100);
        }
    }

    for (int i = 0; i < 4; i++) {
        if (cachedPersistValues.time[i] != -1 && gameTransferListing.timeBytesForTransfer[i] > 0) {
            aa_genesis_setWorkRam(gameTransferListing.timeBytesForTransfer[i] % 0x10000, cachedPersistValues.time[i] % 0x100);

            // if (i == 3) {
            // aa_genesis_setWorkRam(0xFE25, 59);
            // }

            char tempLog[0x100];
            sprintf(tempLog, "Wrting time value (%i) %d at %04X", i, cachedPersistValues.time[i], gameTransferListing.timeBytesForTransfer[i]);
            cartLoader_appendToLog(tempLog); 
            flagHUDtoUpdate();
        }
    }
    
    for (int i = 0; i < 8; i++) {
        if (cachedPersistValues.momentum[i] != -1 && gameTransferListing.momentumBytesForTransfer[i] > 0) {
            aa_genesis_setWorkRam(gameTransferListing.momentumBytesForTransfer[i] % 0x10000, cachedPersistValues.momentum[i] % 0x100);

            char tempLog[0x100];
            sprintf(tempLog, "Wrting momentum value (%i) %d at %04X", i, cachedPersistValues.momentum[i], gameTransferListing.momentumBytesForTransfer[i]);
            cartLoader_appendToLog(tempLog); 
            flagHUDtoUpdate();
        }
    }

    for (int i = 0; i < 4; i++) {
        if (cachedPersistValues.score[i] != -1 && gameTransferListing.scoreBytesForTransfer[i] > 0) {
            aa_genesis_setWorkRam(gameTransferListing.scoreBytesForTransfer[i] % 0x10000, cachedPersistValues.score[i] % 0x100);

            char tempLog[0x100];
            sprintf(tempLog, "Wrting score value (%i) %d at %04X", i, cachedPersistValues.score[i], gameTransferListing.scoreBytesForTransfer[i]);
            cartLoader_appendToLog(tempLog); 
            flagHUDtoUpdate();
        }
    }
}

// static int shouldUpdateHUD = 0;
// int cartLoader_getFlagToUpdateHUD() {
//     return shouldUpdateHUD;
// }

void flagHUDtoUpdate() {
    // shouldUpdateHUD = 1;

    // problem: the timer doesn't update nicely because the "I should change the HUD"
    //          flag is set in the game's code, and is based on the seconds timer

    AAGameListing gameListing = cartLoader_getActiveGameListing();

    for (int i = 0; i < 8; i++) {
        if (gameListing.updateHUDFlags[i] > 0) {
            aa_genesis_setWorkRam(gameListing.updateHUDFlags[i] % 0x10000, 0b10000001);

            char tempLog[0x100];
            sprintf(tempLog, "Updating HUD (%i)at %04X", i, gameListing.updateHUDFlags[i]);
            cartLoader_appendToLog(tempLog); 
        }
    }

    // shouldUpdateHUD = 0;
}

// void cartLoader_UpdateHUD() {
//     AAGameListing gameListing = cartLoader_getActiveGameListing();

//     for (int i = 0; i < 8; i++) {
//         if (gameListing.updateHUDFlags[i] > 0) {
//             aa_genesis_setWorkRam(gameListing.updateHUDFlags[i] % 0x10000, 0xFF);

//             char tempLog[0x100];
//             sprintf(tempLog, "Updating HUD (%i)at %04X", i, gameListing.updateHUDFlags[i]);
//             cartLoader_appendToLog(tempLog); 
//         }
//     }

//     shouldUpdateHUD = 0;
// }

int cartLoader_string32AreEqual(char strA[], char strB[]) {
    for (int i = 0; i < 32; i++) {
        if (strA[i] != strB[i]) {
            return 0;
        }
        if (strA[i] == 0 || strA[0] == '\0') {
            return 1;
        }
    }
    return 1;
}

int cartLoader_getSwapCount() {
    return gameSwapCount;
}