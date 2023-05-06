#include "AACartLoader.h"

#include "shared.h" // <--- this needs to be included at the top of every file, for compiler reasons I don't understand
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include "include/dirent.h"
#include "vdp_render.h"
#include "AAModConsole.h"
#include "AACommonTypes.h"
#include <sys/stat.h>
#include "AALayerRenderer.h"
#include "genesis.h"
#include "AAMenuDisplay.h"

static int MAX_SIMULTANEOUS_BOSSES = 8;

static unsigned int romCount;
static char *folderPath = "_magicbox";
static char romFileNames[MAX_ROMS][0x100];
static char romFilePrefixes[MAX_ROMS][0x100];
static int romsRemovedFromRandomiser[MAX_ROMS];

static char *logLines[0x100];
static unsigned int logLineCount;

static char* debug_lastFileSystemInteraction[0x100];
static int framesSinceLastFileSystemInteraction = 0;

static AAGameListing gameListings[MAX_ROMS];
static AAMusicOverrideListing musicOverrideListings[MAX_ROMS];
static AAStandTriggerListing standTriggerListings[MAX_ROMS];
static AAGameTransferListing gameTransferListings[MAX_ROMS];
static AAScoreMonitorListing scoreMonitorListings[MAX_ROMS];
static AALevelEditListing levelEditListings[MAX_ROMS];
static AAPixelMonitorListing pixelMonitorListings[MAX_ROMS];
static MomentumControlListing momentumControlListings[MAX_ROMS];
static unsigned char gameAltIds[MAX_ROMS][0x80];
static char nameOfTrigger[MAX_ROMS][100];
static int gameListingCount = 0;

static char *terminalNamePerRom[MAX_ROMS];

static BossRushChallengeListing bossRushCallenges[MAX_ROMS];
static BossRushProgress bossRushProgress[MAX_ROMS];
static int bossRushChallengeCount = 0;

static unsigned char romHeaderBuffer[0x20];

static int lastPixelStatesPerGame[MAX_ROMS][0x100];
static int pixelStatesPerGame[MAX_ROMS][0x100];

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
static int hasBeenNonSMS = 0;

static int foundZipFiles = 0;

static int cachedCartIndex = 0;

static int maxRewindStatesPerGame = 0x20;
static int rewindStateMinimumPerGame[MAX_ROMS];
static int rewindStateCounterPerGame[MAX_ROMS];

static int activeBossRushes[MAX_ROMS];
static int currentBossRushIndex = 0;
static uint8 bossRushSaveStates[MAX_ROMS][STATE_SIZE];
static uint8 hasBossRushSaveState[MAX_ROMS];

static int bossRushRingCarryValue[2];
static int bossRushRingCarryTotal = 0;
static int bossRushSwitchCount = 0;

static int cheatFlagsPerBossRush[MAX_ROMS][8];


char* getNameOfTriggerForGame(int cartIndex) {
    return nameOfTrigger[cartIndex];
}

char* cartLoader_getNameOfTriggerForActiveGame() {
    return getNameOfTriggerForGame(cartLoader_getActiveCartIndex());
}

int cartLoader_base10CharToInt(char character) {
    if (character == '0') {
        return 0;
    }
    if (character == '1') {
        return 1;
    }
    if (character == '2') {
        return 2;
    }
    if (character == '3') {
        return 3;
    }
    if (character == '4') {
        return 4;
    }
    if (character == '5') {
        return 5;
    }
    if (character == '6') {
        return 6;
    }
    if (character == '7') {
        return 7;
    }
    if (character == '8') {
        return 8;
    }
    if (character == '9') {
        return 9;
    }

    return -1;
}

int cartLoader_base10Array32ToInt(char array32[]) {
    int length = 0;
    for (int i = 0; i < 32; i++) {
        if (array32[i] == 0 || array32[0] == '\0') {
            length = i + 1;
            break;
        }
    }

    int runningValue = 0;
    for (int i = length - 1; i >= 0; i--) {
        int placeValue = 1;
        for (int j = 0; j < i; j++) {
            placeValue *= 10;
        }
        runningValue = placeValue * cartLoader_base10CharToInt(array32[i]);
    }

    return runningValue;
}

void writeFolderPathIntoArray32(char array32[]) {
    writeStringToArray32(folderPath, array32);
}

void initialiseBossRush() {
    populateBossRushes();

    for (int i = 0; i < MAX_ROMS; i++) {
        activeBossRushes[i] = -1;
    }
}

void cartLoader_run() {
    for (int i = 0; i < MAX_ROMS; i++) {
        hasCachedSaveState[i] = 0;
        romsRemovedFromRandomiser[i] = 0;
        rewindStateMinimumPerGame[i] = 0;
        rewindStateCounterPerGame[i] = 0;
    }
    
    cartLoader_appendToLog("cartLoader_run");

    initialiseDirectory();

    cartLoader_appendToLog("will list files");

    listFiles(folderPath, "/");

    cartLoader_appendToLog("Listed files");

    zeroAllListings();

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
    gameListings[0].isISO = 0;
    gameListings[0].ringSwitchCooldown = 0;

    writeStringToArray32("SONICTHEHEDGEHOG", gameListings[1].gameId);// = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','\0'};
    terminalNamePerRom[1] = "Sonic the Hedgehog";
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
    levelEditListings[1].startByte = 0xA408;
    levelEditListings[1].endByte = 0xA800;
    gameListings[1].unpauseByte = 0xF604; //0xF63A;
    gameListings[1].unpauseByteDestination = 0x80; // 0x1;
    momentumControlListings[1].radius = 0x08;
    momentumControlListings[1].xByteStart = 0xD011;
    momentumControlListings[1].yByteStart = 0xD013;
    momentumControlListings[1].inertiaByte = 0xD015;
    momentumControlListings[1].inertiaMin = 0x2;
    momentumControlListings[1].inertiaMax = 0xC;
    gameTransferListings[1].gameStateByte = 0xF601;
    gameTransferListings[1].gameStatesToBlockSwitch[0] = 0x00; // SEGA SCREEN
    gameTransferListings[1].gameStatesToBlockSwitch[1] = 0x04; // TITLE SCREEN
    gameTransferListings[1].gameStatesToBlockSwitch[2] = 0x14; // CONTINUE SCREEN
    gameTransferListings[1].gameStatesToBlockScramble[0] = 0x00; // SEGA SCREEN
    gameTransferListings[1].gameStatesToBlockScramble[1] = 0x04; // TITLE SCREEN
    gameTransferListings[1].gameStatesToBlockScramble[2] = 0x10; // SPECIAL STAGE
    gameTransferListings[1].gameStatesToBlockScramble[3] = 0x14; // CONTINUE SCREEN
    gameListings[1].valueWriteDuration = 0;//60;
    scoreMonitorListings[1].allowStackRingInputs = 1;
    standTriggerListings[1].standingByte = 0xD023;
    standTriggerListings[1].standingBit = 1;
    standTriggerListings[1].standingRequiredValue = 0;
    standTriggerListings[1].standingCooldown = 5;
    sprintf(nameOfTrigger[1], "Sonic gets a ring");
    musicOverrideListings[1].shouldEditZ80 = 0;
    musicOverrideListings[1].byteToCheckForTrackChange = 0xF003;
    musicOverrideListings[1].valueToWriteIntoTrackChangedSlot = 0;
    musicOverrideListings[1].byteToWriteToForNoMusic = 0xF008;
    musicOverrideListings[1].valueToWriteForNoMusic = 0;
    musicOverrideListings[1].applyChangeDuration = 5;

    writeStringToArray32("SONICTHEHEDGEHOG2", gameListings[2].gameId);//gameListings[1].gameId = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','2','\0'};
    terminalNamePerRom[2] = "Sonic the Hedgehog 2";
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
    levelEditListings[2].startByte = 0x8008;
    levelEditListings[2].endByte = 0x9000;
    momentumControlListings[2].xByteStart = 0xB011;
    momentumControlListings[2].yByteStart = 0xB013;
    momentumControlListings[2].inertiaByte = 0xB015;
    standTriggerListings[2].standingByte = 0xB023;
    standTriggerListings[2].standingBit = 1;
    standTriggerListings[2].standingRequiredValue = 0;
    standTriggerListings[2].standingCooldown = 5;
    musicOverrideListings[2].shouldEditZ80 = 1;
    musicOverrideListings[2].byteToCheckForTrackChange = 0x1B82;
    musicOverrideListings[2].valueToWriteIntoTrackChangedSlot = 0;
    musicOverrideListings[2].byteToWriteToForNoMusic = 0x1B88;
    musicOverrideListings[2].valueToWriteForNoMusic = 0;
    musicOverrideListings[2].applyChangeDuration = 5;

    writeStringToArray32("SONICTHEHEDGEHOG3", gameListings[3].gameId);//gameListings[2].gameId = {'S','O','N','I','C','T','H','E','H','E','D','G','E','H','O','G','3','\0'};
    terminalNamePerRom[3] = "Sonic the Hedgehog 3";
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
    levelEditListings[3].startByte = 0x8008;
    levelEditListings[3].endByte = 0x9000;
    momentumControlListings[3].xByteStart = 0xB019;
    momentumControlListings[3].yByteStart = 0xB01B;
    momentumControlListings[3].inertiaByte = 0xB01D;
    gameTransferListings[3].gameStateByte = 0xF601;
    // when to block game switch (i.e. menu screens)
    gameTransferListings[3].gameStatesToBlockSwitch[0] = 0x00; // SEGA SCREEN
    gameTransferListings[3].gameStatesToBlockSwitch[1] = 0x04; // TITLE SCREEN
    gameTransferListings[3].gameStatesToBlockSwitch[2] = 0x14; // CONTINUE SCREEN
    gameTransferListings[3].gameStatesToBlockSwitch[3] = 0x1C; // LEVEL SELECT
    gameTransferListings[3].gameStatesToBlockSwitch[4] = 0x24; // LEVEL SELECT
    gameTransferListings[3].gameStatesToBlockSwitch[5] = 0x28; // LEVEL SELECT
    gameTransferListings[3].gameStatesToBlockSwitch[6] = 0x38; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockSwitch[7] = 0x3C; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockSwitch[8] = 0x40; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockSwitch[9] = 0x44; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockSwitch[10] = 0x4C; // FILE SELECT
    // when to block game scramble (i.e. menu screens and special stages)
    gameTransferListings[3].gameStatesToBlockScramble[0] = 0x00; // SEGA SCREEN
    gameTransferListings[3].gameStatesToBlockScramble[1] = 0x04; // TITLE SCREEN
    gameTransferListings[3].gameStatesToBlockScramble[2] = 0x14; // CONTINUE SCREEN
    gameTransferListings[3].gameStatesToBlockScramble[3] = 0x1C; // LEVEL SELECT
    gameTransferListings[3].gameStatesToBlockScramble[4] = 0x24; // LEVEL SELECT
    gameTransferListings[3].gameStatesToBlockScramble[5] = 0x28; // LEVEL SELECT
    gameTransferListings[3].gameStatesToBlockScramble[6] = 0x38; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockScramble[7] = 0x3C; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockScramble[8] = 0x40; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockScramble[9] = 0x44; // VS MODE MENU
    gameTransferListings[3].gameStatesToBlockScramble[10] = 0x2C; // BLUE SPHERES
    gameTransferListings[3].gameStatesToBlockScramble[11] = 0x34; // SPECIAL STAGE
    gameTransferListings[3].gameStatesToBlockScramble[12] = 0x48; // SPECIAL STAGE RESULTS
    gameTransferListings[3].gameStatesToBlockScramble[13] = 0x4C; // FILE SELECT
    standTriggerListings[3].standingByte = 0xB02B;
    standTriggerListings[3].standingBit = 1;
    standTriggerListings[3].standingRequiredValue = 0;
    standTriggerListings[3].standingCooldown = 5;
    musicOverrideListings[3].shouldEditZ80 = 1;
    musicOverrideListings[3].byteToCheckForTrackChange = 0; //0x1C34;
    musicOverrideListings[3].valueToWriteIntoTrackChangedSlot = 0x00;
    musicOverrideListings[3].byteStringCheckForTrackChange = 4;
    musicOverrideListings[3].byteToWriteToForNoMusic = 0x1FF4; //0x1FF2; //0x1FF4;
    musicOverrideListings[3].valueToWriteForNoMusic = 0x00;
    musicOverrideListings[3].secondByteToWriteToForNoMusic = 0x1FF5; //0x1FF3; //0x1FF5;
    musicOverrideListings[3].secondValueToWriteForNoMusic = 0x00;
    // musicOverrideListings[3].byteStringLengthToWriteForNoMusic = 4;
    musicOverrideListings[3].applyChangeDuration = 180;

    writeStringToArray32("SONIC&KNUCKLES", gameListings[4].gameId);//gameListings[3].gameId = {'S','O','N','I','C','&','K','N','U','C','K','L','E','S','\0'};
    terminalNamePerRom[4] = "Sonic & Knuckles";
    copyGameListing(3, 4);

    writeStringToArray32("SONIC3&KNUCKLES", gameListings[5].gameId);
    terminalNamePerRom[5] = "Sonic 3 & Knuckles";
    copyGameListing(3, 5);

    writeStringToArray32("SONIC3DBLAST", gameListings[6].gameId);
    terminalNamePerRom[6] = "Sonic 3D Blast";
    gameListings[6].ringByte = 0x0A5A; // <-- it's a weird number!! 
    gameListings[6].specialRingByte = 0xA17C;    // <-- to do!
    gameListings[6].livesBytes[0] = 0x0680;
    gameListings[6].livesByteDestinations[0] = 0x5; 
    gameListings[6].accelerationType = 2;
    gameListings[6].valueWriteDuration = 60; // once per second
    scoreMonitorListings[6].allowStackRingInputs = 1;
    sprintf(nameOfTrigger[6], "Sonic gets a ring");
    gameTransferListings[6].ringBytesForTransfer[0] = 0x0A56; // low byte
    gameTransferListings[6].ringBytesForTransfer[1] = 0x0A57; // high byte

    writeStringToArray32("SonicSpinball", gameListings[7].gameId);
    terminalNamePerRom[7] = "Sonic  Spinball";
    gameListings[7].ringByte = 0x57A1;
    gameListings[7].specialRingByte = 0;    
    gameListings[7].livesBytes[0] = 0x579F;
    gameListings[7].livesByteDestinations[0] = 0x5; 
    gameListings[7].valueWriteDuration = 300; // only update lives every 20 seconds
    sprintf(nameOfTrigger[7], "Sonic gets a ring");

    writeStringToArray32("15900", gameListings[8].gameId); // Sonic 2 MS
    terminalNamePerRom[8] = "Sonic the Hedgehog 2 (SMS)";
    gameListings[8].ringByte = 0x1299;
    gameListings[8].specialRingByte = 0;    
    gameListings[8].livesBytes[0] = 0x1298;
    gameListings[8].livesByteDestinations[0] = 0x5; 
    gameListings[8].timeBytes[0] = 0x12B9;
    gameListings[8].timeByteDestinations[0] = 1;
    gameListings[8].timeBytes[1] = 0x12BA;
    gameListings[8].timeByteDestinations[1] = 1;
    gameListings[8].valueWriteDuration = 60;
    gameListings[8].panicBytes[0] = 0x12B9;
    gameListings[8].panicByteDestinations[0] = 0x59;
    gameListings[8].panicBytes[1] = 0x12BA;
    gameListings[8].panicByteDestinations[1] = 0x09;
    gameListings[8].valueWriteDuration = 300; // only update lives every 20 seconds
    scoreMonitorListings[8].allowStackRingInputs = 1;
    levelEditListings[8].startByte = 0x0001;
    levelEditListings[8].endByte = 0x1000;
    sprintf(nameOfTrigger[8], "Sonic gets a ring");

    writeStringToArray32("76700", gameListings[9].gameId); // Sonic 1 MS
    terminalNamePerRom[9] = "Sonic the Hedgehog (SMS)";
    gameListings[9].ringByte = 0x12AA;
    gameListings[9].specialRingByte = 0;    
    gameListings[9].livesBytes[0] = 0x1246;
    gameListings[9].livesByteDestinations[0] = 0x5; 
    gameListings[9].timeBytes[0] = 0x12CE;
    gameListings[9].timeByteDestinations[0] = 1;
    gameListings[9].valueWriteDuration = 60;
    gameListings[9].panicBytes[0] = 0x1402;
    gameListings[9].panicByteDestinations[0] = 0xFF;
    gameListings[9].valueWriteDuration = 600; // only update lives every 20 seconds
    scoreMonitorListings[9].allowStackRingInputs = 1;
    levelEditListings[9].startByte = 0x0001;
    levelEditListings[9].endByte = 0x1000;
    sprintf(nameOfTrigger[9], "Sonic gets a ring");

    writeStringToArray32("21900", gameListings[10].gameId); // Sonic Chaos MS
    terminalNamePerRom[10] = "Sonic Chaos (SMS)";
    gameListings[10].ringByte = 0x129A;
    gameListings[10].specialRingByte = 0;    
    gameListings[10].livesBytes[0] = 0x1299;
    gameListings[10].livesByteDestinations[0] = 0x5; 
    gameListings[10].timeBytes[0] = 0x12C0;
    gameListings[10].timeByteDestinations[0] = 1;
    gameListings[10].valueWriteDuration = 60;
    gameListings[10].panicBytes[0] = 0x12BF;
    gameListings[10].panicByteDestinations[0] = 0x59;
    gameListings[10].panicBytes[1] = 0x12C0;
    gameListings[10].panicByteDestinations[1] = 0x09;
    gameListings[10].valueWriteDuration = 600; // only update lives every 20 seconds
    scoreMonitorListings[10].allowStackRingInputs = 1;
    levelEditListings[10].startByte = 0x0001;
    levelEditListings[10].endByte = 0x1000;
    sprintf(nameOfTrigger[10], "Sonic gets a ring");

    writeStringToArray32("73250", gameListings[11].gameId); // Sonic Blast MS <-- still need to find lives and time
    terminalNamePerRom[11] = "Sonic Blast (SMS)";
    gameListings[11].ringByte = 0x125E; // <-- WARNING! it is used in the title sequence (once per frame?)
    gameListings[11].specialRingByte =  0x1D9E;
    gameListings[11].livesBytes[0] = 0x1178;
    gameListings[11].livesByteDestinations[0] = 0x5; 
    gameListings[11].timeBytes[0] = 0;
    gameListings[11].timeByteDestinations[0] = 1;
    gameListings[11].valueWriteDuration = 60;
    gameListings[11].ringSwitchCooldown = 8;
    sprintf(nameOfTrigger[11], "Sonic gets a ring");

    writeStringToArray32("07250", gameListings[12].gameId); // Sonic 2 GG
    terminalNamePerRom[12] = "Sonic the Hedgehog 2 (GG)";
    gameListings[12].ringByte = 0x1299;
    gameListings[12].specialRingByte = 0;
    gameListings[12].livesBytes[0] = 0x1298;
    gameListings[12].livesByteDestinations[0] = 0x5; 
    gameListings[12].timeBytes[0] = 0x12BA;
    gameListings[12].timeByteDestinations[0] = 1;
    gameListings[12].valueWriteDuration = 60;    
    scoreMonitorListings[12].allowStackRingInputs = 1;
    sprintf(nameOfTrigger[12], "Sonic gets a ring");

    writeStringToArray32("08240", gameListings[13].gameId); // Sonic 1 GG
    terminalNamePerRom[13] = "Sonic the Hedgehog (GG)";
    gameListings[13].ringByte = 0x12A9;
    gameListings[13].specialRingByte = 0;
    gameListings[13].livesBytes[0] = 0x1240;
    gameListings[13].livesByteDestinations[0] = 0x5; 
    gameListings[13].timeBytes[0] = 0x12CF;
    gameListings[13].timeByteDestinations[0] = 1;
    gameListings[13].valueWriteDuration = 60;   
    scoreMonitorListings[13].allowStackRingInputs = 1;
    sprintf(nameOfTrigger[13], "Sonic gets a ring");

    writeStringToArray32("15250", gameListings[14].gameId); // Sonic Chaos GG
    terminalNamePerRom[14] = "Sonic Chaos (GG)";
    gameListings[14].ringByte = 0x129C;
    gameListings[14].specialRingByte = 0;
    gameListings[14].livesBytes[0] = 0x129B;
    gameListings[14].livesByteDestinations[0] = 0x5; 
    gameListings[14].timeBytes[0] = 0x12C2;
    gameListings[14].timeByteDestinations[0] = 1;
    gameListings[14].valueWriteDuration = 60;   
    scoreMonitorListings[14].allowStackRingInputs = 1;
    sprintf(nameOfTrigger[14], "Sonic gets a ring");

    writeStringToArray32("73250", gameListings[15].gameId); // Sonic Blast GG <-- still need to find lives and time (identical to SMS)
    terminalNamePerRom[15] = "Sonic Blast (GG)";
    copyGameListing(11, 15);

    writeStringToArray32("30250", gameListings[16].gameId); // Sonic Triple Trouble<-- still need to find lives and time
    terminalNamePerRom[16] = "Sonic Triple Trouble (GG)";
    gameListings[16].ringByte = 0x1159;
    gameListings[16].specialRingByte = 0;
    gameListings[16].livesBytes[0] = 0x1140;
    gameListings[16].livesByteDestinations[0] = 0x5; 
    gameListings[16].timeBytes[0] = 0x115E;
    gameListings[16].timeByteDestinations[0] = 1;
    gameListings[16].valueWriteDuration = 60;   
    scoreMonitorListings[16].allowStackRingInputs = 1;
    sprintf(nameOfTrigger[16], "Sonic gets a ring");

    
    writeStringToArray32("SONICCD", gameListings[17].gameId); // <-- not used yet
    gameListings[17].ringByte = 0;
    gameListings[17].specialRingByte = 0;
    gameListings[17].livesBytes[0] = 0;
    gameListings[17].livesByteDestinations[0] = 0x5; 
    gameListings[17].timeBytes[0] = 0;
    gameListings[17].timeByteDestinations[0] = 1;
    gameListings[17].valueWriteDuration = 60;   
    gameListings[17].isISO = 1;  // <-- I plan to use this as a way to detect CD games for the time being...
    sprintf(nameOfTrigger[17], "Sonic gets a ring");

    writeStringToArray32("Dr.Robotnik'sMeanBeanMachine", gameListings[18].gameId); // <-- not used yet
    terminalNamePerRom[18] = "Dr Robotnik's Mean Bean Machine";
    gameListings[18].ringByte = 0;
    gameListings[18].specialRingByte = 0;
    gameListings[18].livesBytes[0] = 0;
    gameListings[18].livesByteDestinations[0] = 0x5; 
    gameListings[18].timeBytes[0] = 0;
    gameListings[18].timeByteDestinations[0] = 1;
    gameListings[18].valueWriteDuration = 0;
    scoreMonitorListings[18].scoreBytes[0] = 0xE00C;
    scoreMonitorListings[18].scoreBytes[1] = 0xE00D;
    scoreMonitorListings[18].scoreBytesP2[0] = 0xE04C;
    scoreMonitorListings[18].scoreBytesP2[1] = 0xE04D;
    scoreMonitorListings[18].calculatationType = 0;
    scoreMonitorListings[18].scoreJumpForTrigger = 39;
    scoreMonitorListings[18].blockJumpFromZero = 1;
    // gameListings[18].ringSwitchCooldown = 2;
    gameListings[18].postRingEffectCooldown = 20;
    sprintf(nameOfTrigger[18], "beans are popped");


    writeStringToArray32("Puyo Puyo (JP)", gameListings[19].gameId); // <-- puyo puyo
    terminalNamePerRom[19] = "Puyo Puyo";
    copyGameListing(18, 19);
    sprintf(gameAltIds[19], "82D582E682D582E6 0000000000000000 0000000000000000 0000000000000000");
    gameListings[19].ringByte = 0;
    gameListings[19].specialRingByte = 0;
    sprintf(nameOfTrigger[19], "beans are popped");

    writeStringToArray32("Puyo Puyo 2 (JP)", gameListings[20].gameId); // <-- puyo puyo 2
    sprintf(gameAltIds[20], "82D582E682D582E6 8251000000000000 0000000000000000 0000000000000000");
    terminalNamePerRom[20] = "Puyo Puyo 2";
    gameListings[20].ringByte = 0;
    gameListings[20].specialRingByte = 0;
    gameListings[20].livesBytes[0] = 0;
    gameListings[20].livesByteDestinations[0] = 0x5; 
    gameListings[20].timeBytes[0] = 0;
    gameListings[20].timeByteDestinations[0] = 1;
    gameListings[20].valueWriteDuration = 0;
    scoreMonitorListings[20].scoreBytes[0] = 0xD08C;
    scoreMonitorListings[20].scoreBytes[1] = 0xD08D;
    scoreMonitorListings[20].scoreBytesP2[0] = 0xD0CC;
    scoreMonitorListings[20].scoreBytesP2[1] = 0xD0CD;
    scoreMonitorListings[20].calculatationType = 0;
    scoreMonitorListings[20].scoreJumpForTrigger = 39;
    scoreMonitorListings[20].blockJumpFromZero = 1;
    // gameListings[20].ringSwitchCooldown = 2;
    gameListings[20].postRingEffectCooldown = 20;
    sprintf(nameOfTrigger[20], "beans are popped");

    writeStringToArray32("BAREKNUCKLE", gameListings[21].gameId); // <-- Streets of Rage 1
    terminalNamePerRom[21] = "Streets of Rage";
    gameListings[21].ringByte = 0;
    gameListings[21].specialRingByte = 0;
    gameListings[21].livesBytes[0] = 0xFF21;
    gameListings[21].livesByteDestinations[0] = 0x5; 
    gameListings[21].timeBytes[0] = 0xFB00;
    gameListings[21].timeByteDestinations[0] = 50;
    gameListings[21].valueWriteDuration = 60;
    scoreMonitorListings[21].scoreBytes[0] = 0xFF0B; // have to use score because SoR 1 doesn't show enemy health
    scoreMonitorListings[21].scoreBytes[1] = 0xFF08;
    scoreMonitorListings[21].scoreBytesP2[0] = 0;
    scoreMonitorListings[21].scoreBytesP2[1] = 0;
    scoreMonitorListings[21].calculatationType = 1;
    scoreMonitorListings[21].scoreJumpForTrigger = 1;
    gameListings[21].ringSwitchCooldown = 2;
    sprintf(nameOfTrigger[21], "an enemy is defeated");

    writeStringToArray32("BAREKNUCKLE2", gameListings[22].gameId); // <-- Streets of Rage 2
    terminalNamePerRom[22] = "Streets of Rage 2";
    gameListings[22].ringByte = 0;
    gameListings[22].specialRingByte = 0;
    gameListings[22].livesBytes[0] = 0xEF82;
    gameListings[22].livesByteDestinations[0] = 0x5; 
    gameListings[22].timeBytes[0] = 0;
    gameListings[22].timeByteDestinations[0] = 1;
    gameListings[22].valueWriteDuration = 60;
    // scoreMonitorListings[22].scoreBytes[0] = 0xEF99;
    // scoreMonitorListings[22].scoreBytes[1] = 0xEF96;
    // scoreMonitorListings[22].scoreBytesP2[0] = 0;
    // scoreMonitorListings[22].scoreBytesP2[1] = 0;
    // scoreMonitorListings[22].calculatationType = 1;
    // scoreMonitorListings[22].scoreJumpForTrigger = 99;
    gameListings[22].ringSwitchCooldown = 2;
    pixelMonitorListings[22].xCoords[0] = 0x18; // the last pixel of the
    pixelMonitorListings[22].yCoords[0] = 0x24; // health bar
    pixelMonitorListings[22].allowedColours[0] = 0x46; // red
    pixelMonitorListings[22].allowedColours[1] = 0x4D; // yellow
    pixelMonitorListings[22].changeMustAffectColour = 0x46; // <--- only act when yellow enemy health turns red
    pixelMonitorListings[22].enabled = 1; 
    sprintf(nameOfTrigger[22], "an enemy is defeated");
    
    writeStringToArray32("BAREKNUCKLE3", gameListings[23].gameId); // <-- Streets of Rage 3
    terminalNamePerRom[23] = "Streets of Rage 3";
    gameListings[23].ringByte = 0;
    gameListings[23].specialRingByte = 0;
    gameListings[23].livesBytes[0] = 0xDF8A;
    gameListings[23].livesByteDestinations[0] = 0x5; 
    gameListings[23].timeBytes[0] = 0;
    gameListings[23].timeByteDestinations[0] = 1;
    gameListings[23].valueWriteDuration = 60;
    // scoreMonitorListings[23].scoreBytes[0] = 0xDF82;
    // scoreMonitorListings[23].scoreBytes[1] = 0xDF83;
    // scoreMonitorListings[23].scoreBytesP2[0] = 0;
    // scoreMonitorListings[23].scoreBytesP2[1] = 0;
    // scoreMonitorListings[23].calculatationType = 0;
    // scoreMonitorListings[23].scoreJumpForTrigger = 99;
    gameListings[23].ringSwitchCooldown = 2;
    pixelMonitorListings[23].xCoords[0] = 0x18; // the last pixel of the
    pixelMonitorListings[23].yCoords[0] = 0x24; // health bar
    pixelMonitorListings[23].allowedColours[0] = 0x46; // red
    pixelMonitorListings[23].allowedColours[1] = 0x4D; // yellow
    pixelMonitorListings[23].changeMustAffectColour = 0x46; // <--- only act when yellow enemy health turns red
    pixelMonitorListings[23].enabled = 1; 
    sprintf(nameOfTrigger[23], "an enemy is defeated");


    writeStringToArray32("THESUPERSHINOBI2", gameListings[24].gameId); // <-- Shinobi III
    terminalNamePerRom[24] = "Shinobi III";
    gameListings[24].ringByte = 0;
    gameListings[24].specialRingByte = 0;
    gameListings[24].livesBytes[0] = 0x37E1;
    gameListings[24].livesBytes[1] = 0x37CD;
    gameListings[24].livesByteDestinations[0] = 0x5; 
    gameListings[24].livesByteDestinations[1] = 0x5; 
    gameListings[24].timeBytes[0] = 0;
    gameListings[24].timeByteDestinations[0] = 1;
    gameListings[24].valueWriteDuration = 60;
    scoreMonitorListings[24].scoreBytes[0] = 0x37B4;
    scoreMonitorListings[24].scoreBytes[1] = 0x37B5;
    scoreMonitorListings[24].scoreBytes[2] = 0x37B6;
    scoreMonitorListings[24].scoreBytesP2[0] = 0;
    scoreMonitorListings[24].scoreBytesP2[1] = 0;
    scoreMonitorListings[24].calculatationType = 1;
    scoreMonitorListings[24].scoreJumpForTrigger = 1;
    gameListings[24].ringSwitchCooldown = 2;
    sprintf(nameOfTrigger[24], "an enemy is defeated");

    writeStringToArray32("THESUPERSHINOBI", gameListings[25].gameId); // <-- Revenge of Shinobi
    terminalNamePerRom[25] = "Revenge of Shinobi";
    gameListings[25].ringByte = 0;
    gameListings[25].specialRingByte = 0;
    gameListings[25].livesBytes[0] = 0xE140;
    gameListings[25].livesByteDestinations[0] = 0x5; 
    gameListings[25].timeBytes[0] = 0;
    gameListings[25].timeByteDestinations[0] = 1;
    gameListings[25].valueWriteDuration = 60;
    scoreMonitorListings[25].scoreBytes[0] = 0xFF12;
    scoreMonitorListings[25].scoreBytes[1] = 0xFF13;
    scoreMonitorListings[25].scoreBytesP2[0] = 0;
    scoreMonitorListings[25].scoreBytesP2[1] = 0;
    scoreMonitorListings[25].calculatationType = 1;
    scoreMonitorListings[25].scoreJumpForTrigger = 1;
    gameListings[25].ringSwitchCooldown = 2;
    sprintf(nameOfTrigger[25], "an enemy is defeated");

    writeStringToArray32("SHADOWDANCER", gameListings[26].gameId); // <-- SHADOWDANCER
    sprintf(gameAltIds[26], "534841444F574441 4E434552896582CC 9591000000000000 0000000000000000");
    terminalNamePerRom[26] = "Shadow Dancer";
    gameListings[26].ringByte = 0;
    gameListings[26].specialRingByte = 0;
    gameListings[26].livesBytes[0] = 0x13DF;
    gameListings[26].livesByteDestinations[0] = 0x5; 
    gameListings[26].timeBytes[0] = 0;
    gameListings[26].timeByteDestinations[0] = 1;
    gameListings[26].valueWriteDuration = 60;
    scoreMonitorListings[26].scoreBytes[0] = 0x13E3;
    scoreMonitorListings[26].scoreBytes[1] = 0x13E0;
    scoreMonitorListings[26].scoreBytesP2[0] = 0;
    scoreMonitorListings[26].scoreBytesP2[1] = 0;
    scoreMonitorListings[26].calculatationType = 1;
    scoreMonitorListings[26].scoreJumpForTrigger = 0;
    gameListings[26].ringSwitchCooldown = 2;
    sprintf(nameOfTrigger[26], "an enemy is defeated");

    writeStringToArray32("MICROMACHINESII", gameListings[27].gameId); // <-- SHADOWDANCER
    terminalNamePerRom[27] = "Micro Machines 2";
    pixelMonitorListings[27].xCoords[0] = 0x1C;
    pixelMonitorListings[27].xCoords[1] = 0x1C;
    pixelMonitorListings[27].xCoords[2] = 0x1C;
    pixelMonitorListings[27].xCoords[3] = 0x1C;
    pixelMonitorListings[27].yCoords[0] = 0x1D; // first helmet
    pixelMonitorListings[27].yCoords[1] = 0x30; // second helmet
    pixelMonitorListings[27].yCoords[2] = 0x42; // third helmet
    pixelMonitorListings[27].yCoords[3] = 0x54; // fourth helmet
    pixelMonitorListings[27].allowedColours[0] = 0xB3; // blue helmet
    pixelMonitorListings[27].allowedColours[1] = 0xA3; // green helmet
    pixelMonitorListings[27].allowedColours[2] = 0xB7; // red helmet
    pixelMonitorListings[27].allowedColours[3] = 0xA7; // yellow helmet
    pixelMonitorListings[27].changeMustAffectColour = 0xB7; // <--- only act when red changes position
    pixelMonitorListings[27].enabled = 1; // not on because it is disappointing
    gameListings[27].bytesToTestForChange[0] = 0xD964; // swap on new lap
    gameListings[27].bytesToTestForChange[1] = 0xD77E; // swap on battle score change
    gameListings[27].ringSwitchCooldown = 15;
    gameListings[27].livesBytes[0] = 0xF330;
    gameListings[27].livesByteDestinations[0] = 0x5; 
    sprintf(nameOfTrigger[27], "you change race position");


    writeStringToArray32("MicroMachines96", gameListings[28].gameId); // <-- Micro Machines 96 (has no header???)
    copyGameListing(27, 28);
    terminalNamePerRom[28] = "Micro Machines 96";
    // the below comes from the fingerprint
    sprintf(gameAltIds[28], "393AFFFF9AE6C18A 052C4548784AA8CF 0667FCCA18000460 FCCA200045000000");
    gameListings[28].bytesToTestForChange[0] = 0xD164; // swap on new lap
    gameListings[28].bytesToTestForChange[1] = 0xCF7E; // swap on battle score change
    gameListings[28].livesBytes[0] = 0xE776;
    gameListings[28].livesByteDestinations[0] = 0x5; 
    sprintf(nameOfTrigger[28], "you change race position");

    writeStringToArray32("MicroMachines", gameListings[29].gameId); // <-- Micro Machines 96 (has no header???)
    sprintf(gameAltIds[29], "569A754E0061BAFF B94E000086A73C36 0000F94100001D9F 3C300E003C000000");
    terminalNamePerRom[28] = "Micro Machines";
    pixelMonitorListings[29].xCoords[0] = 0x1C;
    pixelMonitorListings[29].xCoords[1] = 0x1C;
    pixelMonitorListings[29].xCoords[2] = 0x1C;
    pixelMonitorListings[29].xCoords[3] = 0x1C;
    pixelMonitorListings[29].yCoords[0] = 0x26; // first helmet
    pixelMonitorListings[29].yCoords[1] = 0x36; // second helmet
    pixelMonitorListings[29].yCoords[2] = 0x46; // third helmet
    pixelMonitorListings[29].yCoords[3] = 0x56; // fourth helmet
    pixelMonitorListings[29].allowedColours[0] = 0xA5; // blue helmet
    pixelMonitorListings[29].allowedColours[1] = 0xB5; // green helmet
    pixelMonitorListings[29].allowedColours[2] = 0xA9; // red helmet
    pixelMonitorListings[29].allowedColours[3] = 0xB8; // yellow helmet
    pixelMonitorListings[29].changeMustAffectColour = 0xB8; // <--- only act when red changes position
    pixelMonitorListings[29].enabled = 1; // not on because it is disappointing
    gameListings[29].bytesToTestForChange[0] = 0xA69E; // swap on new lap
    gameListings[29].bytesToTestForChange[1] = 0xA6C0; // swap on battle score change
    gameListings[29].ringSwitchCooldown = 15;
    gameListings[29].livesBytes[0] = 0xA6C6;
    gameListings[29].livesByteDestinations[0] = 0x5; 
    gameListings[29].valueWriteDuration = 300;
    sprintf(nameOfTrigger[29], "you change race position");

    writeStringToArray32("STREETSOFRAGE3", gameListings[30].gameId); // <-- Streets of Rage 3
    copyGameListing(23, 30);

    writeStringToArray32("STREETSOFRAGE2", gameListings[31].gameId); // <-- Streets of Rage 3
    copyGameListing(22, 31);

    writeStringToArray32("STREETSOFRAGE", gameListings[32].gameId); // <-- Streets of Rage 3
    copyGameListing(21, 32);

    writeStringToArray32("AfterBurnerII", gameListings[33].gameId);
    sprintf(gameAltIds[33], "B1CCC0B0CADEB0C5 B049490000000000 0000000000000000 0000000000000000");
    gameListings[33].ringByte = 0x06C8;
    gameListings[33].livesBytes[0] = 0x06C2;
    gameListings[33].livesByteDestinations[0] = 0x5; 
    sprintf(nameOfTrigger[33], "you get points");

    writeStringToArray32("GUNSTARHEROES", gameListings[34].gameId);
    terminalNamePerRom[34] = "Gunstar Heroes";
    scoreMonitorListings[34].scoreBytes[0] = 0xA469;
    scoreMonitorListings[34].scoreBytes[1] = 0xA466;
    scoreMonitorListings[34].scoreBytes[2] = 0xA467;
    scoreMonitorListings[34].calculatationType = 1;
    scoreMonitorListings[34].scoreJumpForTrigger = 29;
    gameListings[34].ringSwitchCooldown = 2;
    sprintf(nameOfTrigger[34], "you defeat an enemy");


    writeStringToArray32("26700", gameListings[35].gameId); // Monster World II (Wonder Boy III?)
    scoreMonitorListings[35].scoreBytes[0] = 0x0F55;
    scoreMonitorListings[35].scoreBytes[1] = 0x0F56;
    scoreMonitorListings[35].scoreBytes[2] = 0x0F57;
    scoreMonitorListings[35].scoreBytes[3] = 0x0F58;
    scoreMonitorListings[35].scoreBytes[4] = 0x0F59;
    scoreMonitorListings[35].scoreBytes[5] = 0x0F5A;
    scoreMonitorListings[35].calculatationType = 2;
    scoreMonitorListings[35].scoreJumpForTrigger = 0;
    sprintf(nameOfTrigger[35], "you get points");

    writeStringToArray32("72700", gameListings[36].gameId); // Lucky Dime Caper
    scoreMonitorListings[36].scoreBytes[0] = 0x0005;
    scoreMonitorListings[36].scoreBytes[1] = 0x0004;
    scoreMonitorListings[36].scoreBytes[2] = 0x0003;
    scoreMonitorListings[36].scoreBytes[3] = 0x0002;
    scoreMonitorListings[36].scoreBytes[4] = 0x0001;
    scoreMonitorListings[36].calculatationType = 2;
    scoreMonitorListings[36].scoreJumpForTrigger = 0;
    gameListings[36].livesBytes[0] = 0x0069;
    gameListings[36].livesByteDestinations[0] = 0x5; 
    sprintf(nameOfTrigger[36], "you get points");

    writeStringToArray32("53700", gameListings[37].gameId); // Castle of Illusion
    scoreMonitorListings[37].scoreBytes[0] = 0x0088;
    scoreMonitorListings[37].scoreBytes[1] = 0x0089;
    scoreMonitorListings[37].scoreBytes[2] = 0x008A;
    scoreMonitorListings[37].calculatationType = 1;
    scoreMonitorListings[37].scoreJumpForTrigger = 1;
    gameListings[37].livesBytes[0] = 0x00C8;
    gameListings[37].livesByteDestinations[0] = 0x5; 
    sprintf(nameOfTrigger[37], "you get points");

    writeStringToArray32("ECCO", gameListings[38].gameId);
    terminalNamePerRom[38] = "Ecco the Dolphin";
    scoreMonitorListings[38].scoreBytes[0] = 0xB634; // health
    scoreMonitorListings[38].scoreBytesP2[0] = 0xB636; // air 1
    scoreMonitorListings[38].scoreBytesP2[1] = 0xB637; // air 2
    scoreMonitorListings[38].scoreJumpForTrigger = 2;
    scoreMonitorListings[38].allowNegativeChange = 1;
    sprintf(nameOfTrigger[38], "you get air");

    writeStringToArray32("ECCOTHETIDESOFTIME", gameListings[39].gameId);
    scoreMonitorListings[39].scoreBytes[0] = 0xAA16; // health
    scoreMonitorListings[39].scoreBytesP2[0] = 0xAA18; // air
    scoreMonitorListings[39].scoreBytesP2[1] = 0xAA19; // air 2
    scoreMonitorListings[39].scoreJumpForTrigger = 2;
    scoreMonitorListings[39].allowNegativeChange = 1;
    sprintf(nameOfTrigger[39], "you get air");

    // 08240 = Sonic 1 GG
    // 07250 = Sonic 2 GG
    // 15250 = Sonic Chaos GG
    // 30250 = triple trouble GG
    // 73250 = Blast GG

    // writeStringToArray32("CHAOTIX", gameListings[11].gameId); // Knuckles Chaotix 32x
    // writeStringToArray32("SONICCD", gameListings[11].gameId); // Sonic CD

    gameListingCount = 40;
    cartLoader_appendToLog("finished cartLoader_run");
}

static int bossRushComplete = 0;
static int hasInitialisedBossRush = 0;
static int shouldResetBossRush = 0;

void applyBossRushCachedRings() {
    BossRushOptions bossRushOptions = menuDisplay_getBossRushOptions();
    AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();
    if (shouldUseBossRush()) {
        if (bossRushOptions.carryRingsAcrossGames == 1 && 
            (bossRushOptions.preventCarryInDoomsday == 0 || getActiveBossRushListing().blockRingZeroing == 0)){
            
            if (gameTransferListing.ringCalculatationType == 1) {
                int units = bossRushRingCarryTotal % 10;
                int tens = (bossRushRingCarryTotal / 10) % 10;
                int hundreds = (bossRushRingCarryTotal / 100) % 10;
                int thousands = (bossRushRingCarryTotal / 1000) % 10;

                bossRushRingCarryValue[0] = units + (tens * 0x10);
                bossRushRingCarryValue[1] = hundreds + (thousands * 0x10);
            } else {
                bossRushRingCarryValue[0] = bossRushRingCarryTotal % 0x100;
                bossRushRingCarryValue[1] = bossRushRingCarryTotal / 0x100;
            }
            
            if (gameTransferListing.ringBytesForTransfer[0] > 0) {
                aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[0] % 0x10000, bossRushRingCarryValue[0]);
            }
            if (gameTransferListing.ringBytesForTransfer[1] > 0) {
                aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[1] % 0x10000, bossRushRingCarryValue[1]);
            }

            // Sonic 3D blast - need to copy these values into display
            if (getActiveBossRushListing().gameIndex == 6) {
                // if (bossRushRingCarryValue == 0) {
                //     aa_genesis_setWorkRam(0x0A56, 0);
                //     aa_genesis_setWorkRam(0x0A57, 0);
                //     aa_genesis_setWorkRam(0x0A58, 0);
                //     aa_genesis_setWorkRam(0x0A59, 0);
                //     aa_genesis_setWorkRam(0x0A5A, 0);
                //     aa_genesis_setWorkRam(0x0A5B, 0);
                // } else {
                //     aa_genesis_setWorkRam(gameTransferListing.ringBytesForTransfer[0], (bossRushRingCarryValue[0] + 0xFF) % 0x100);
                //     aa_genesis_setWorkRam(0x0A52, 1);
                // }
            }
        }
    }    
}

int getBossRushComplete() {
    return bossRushComplete;
}

void setShouldResetBossRush(int val) {
    shouldResetBossRush = val;
}

int getShouldShowBossRushAsReadyToReset() {
    if (shouldResetBossRush == 1) {
        return 1;
    }
    if (hasInitialisedBossRush == 0) {
        return 1;
    }
    return 0;
}

int getShouldResetBossRush() {
    return shouldResetBossRush;
}

int getHasInitialisedBossRush() {
    return hasInitialisedBossRush;
}

void setHasInitialisedBossRush(int val) {
    hasInitialisedBossRush = val;
}

void clearBossRushProgress() {
    for (int i = 0; i < MAX_ROMS; i++) {
        bossRushProgress[i].elapsedFrames = 0;
        bossRushProgress[i].zoneId = 0;
        bossRushProgress[i].actId = 0;
        bossRushProgress[i].gameId = 0;
        bossRushProgress[i].isComplete = 0;
        bossRushProgress[i].isFocused = 0;
    }
    saveBossRushProgress();
}

int indexOfBossRushProgress(int gameId, int zoneId, int actId) {
    for (int i = 0; i < MAX_ROMS; i++) {
        if (bossRushProgress[i].gameId == gameId &&
            bossRushProgress[i].zoneId == zoneId &&
            bossRushProgress[i].actId == actId) {
            return i;
        }
    }
    return -1;
}

int indexOfLowestUnusedBossProgressSlot() {
    for (int i = 0; i < MAX_ROMS; i++) {
        if (bossRushProgress[i].gameId == 0) {
            return i;
        }
    }
    return -1;
}

void incrementFrameCountOfActiveBossRush() {
    BossRushChallengeListing listing = getActiveBossRushListing();
    int progressIndex = indexOfBossRushProgress(listing.gameIndex, listing.zoneIndex, listing.actIndex);
    if (progressIndex == -1) {
        progressIndex = indexOfLowestUnusedBossProgressSlot();
        bossRushProgress[progressIndex].gameId = listing.gameIndex;
        bossRushProgress[progressIndex].zoneId = listing.zoneIndex;
        bossRushProgress[progressIndex].actId = listing.actIndex;
    }
    bossRushProgress[progressIndex].elapsedFrames++;

    for (int i = 0; i < MAX_ROMS; i++) {
        bossRushProgress[i].isFocused = 0;
    }
    bossRushProgress[progressIndex].isFocused = 1;

    saveBossRushProgress();
}

void flagActiveBossRushProgressAsComplete() {
    BossRushChallengeListing listing = getActiveBossRushListing();
    int progressIndex = indexOfBossRushProgress(listing.gameIndex, listing.zoneIndex, listing.actIndex);
    if (progressIndex != -1) {
        bossRushProgress[progressIndex].isComplete = 1;
    }
    saveBossRushProgress();
}

void flagAllBossRushProgressAsComplete() {
    for (int i = 0; i < MAX_ROMS; i++) {
        bossRushProgress[i].isComplete = 1;
    }
    saveBossRushProgress();
}

void saveBossRushProgress() {
    if (menuDisplay_getBossRushOptions().shouldExposeTrackerData != 0) {
        char path[0x100];
        sprintf(path, "%s/__bossRushProgress.txt", folderPath);
        FILE *progressWriter = fopen(path, "w");

        if (progressWriter) {
            for (int i = 0; i < MAX_ROMS; i++) {
                if (bossRushProgress[i].gameId != 0) {
                    char text[0x100];
                    sprintf(text, "%i/%i/%i/%i/%i/%i", 
                        bossRushProgress[i].gameId,
                        bossRushProgress[i].zoneId,
                        bossRushProgress[i].actId,
                        bossRushProgress[i].isFocused,
                        bossRushProgress[i].isComplete,
                        bossRushProgress[i].elapsedFrames);

                    fprintf(progressWriter, text);
                    fprintf(progressWriter, "\n");
                }
            }

            BossRushOptions bossOptions = menuDisplay_getBossRushOptions();
            int useMusic = bossOptions.shouldUseExternalMusic;
            // and send the final line "state to be used by music tracker"
            char musicStateText[0x100];
            sprintf(musicStateText, "!!%i/%i/%i/%i/%i", 
                useMusic, // should I play music?
                aa_genesis_getWorkRam(0xF601), // am I on a screen transition? (if this value is <0x80 then switch track)
                getDeathCount(),// am I dying? (if this number changes, fade out)
                rand() % 0x1000,
                getBossRushComplete()
            );
            fprintf(progressWriter, musicStateText);
            fprintf(progressWriter, "\n");
            fclose(progressWriter);
        }
    }
}

int bossRushSwitchRandomNumbers[0x10000]; 
int bossRushSwitchRandomIndex = 0;
void shuffleBossSwitchRandomNumbers() {
    srand(getBossRushSeedWithPrefix(0));
    for (int i = 0; i < 0x10000; i++) {
        bossRushSwitchRandomNumbers[i] = rand();
    }
    bossRushSwitchRandomIndex = 0;
    srand(time(NULL));
}

int getNextRandomBossRushNumber() {
    bossRushSwitchRandomIndex++;
    bossRushSwitchRandomIndex = bossRushSwitchRandomIndex % 0x10000;
    return bossRushSwitchRandomNumbers[bossRushSwitchRandomIndex];
}

void beginBossRush() {
    clearBossRushProgress();
    shuffleBossSwitchRandomNumbers();

    MAX_SIMULTANEOUS_BOSSES = getMaxSimultaneousBosses();
    
    if (hasInitialisedBossRush == 0 || shouldResetBossRush != 0) {
        resetAllBossRushSlots();
        resetBossRushElapsedTimer();
    }

    populateBossRushes();

    cartLoader_loadBossRushSaveStatesFromDisk();
    mapBossRushesToRoms();

    bossRushComplete = 0;
    currentBossRushIndex = -1;
    bumpToNextBossRush();
    cartLoader_cacheSaveStateBeforeMenu();

    vdp_setShouldRandomiseColours(0);

    hasInitialisedBossRush = 1;
    shouldResetBossRush = 0;

    bossRushRingCarryValue[0] = 0;
    bossRushRingCarryValue[1] = 0;
    bossRushRingCarryTotal = 0;
    bossRushSwitchCount = 0;
    gameSwapCount = 0;
    zeroDeathCount();
}

void onBossHit() {
    bumpToNextBossRush();
}

void onBossDefeated() {
    bossRushSwitchCount = 0;
    flagActiveBossRushProgressAsComplete();

    if (currentBossRushIndex > -1) {
        bossRushCallenges[activeBossRushes[currentBossRushIndex]].isCompleted = 1;
        activeBossRushes[currentBossRushIndex] = -1;

        queueBossRushSlots();

        currentBossRushIndex = -1;
        promptSwitchGame();
        fireScreenSnapOnEvent();
    }

    checkForBossRushComplete();
}

int getBossRushIndexInSlot(int slot) {
    return activeBossRushes[slot];
}

int getActiveBossRushSlotId() {
    return currentBossRushIndex;
}

int getActiveBossRushIndex() {
    return activeBossRushes[currentBossRushIndex];
}

BossRushChallengeListing getActiveBossRushListing() {
    int rushToLoad = activeBossRushes[currentBossRushIndex];
    return bossRushCallenges[rushToLoad];
}

void cacheRingCountInBossRush(int becauseOfHit) {
    if (shouldUseBossRush() && hasInitialisedBossRush == 1) {
        BossRushOptions bossRushOptions = menuDisplay_getBossRushOptions();
        AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();
        if (bossRushOptions.carryRingsAcrossGames == 1 && 
            (bossRushOptions.preventCarryInDoomsday == 0 || getActiveBossRushListing().blockRingZeroing == 0)) {

            // in Sonic 3D blast, don't cache if the boss is dead, as we're about to switch,
            // unless we're calling this because of a boss hit
            if (becauseOfHit == 0) {
                cartLoader_appendToLog("Caching rings on game switch");
                BossRushChallengeListing activeListing = getActiveBossRushListing();
                if (activeListing.gameIndex == 6) {
                    return;
                    // int bossIsAlive = 0;
                    // if (aa_genesis_getWorkRam(activeListing.objectLocationStart) != 0) {
                    //     bossIsAlive = 1;

                    //     char bossAliveLog[0x100];
                    //     sprintf(bossAliveLog, "----- Sonic 3D: boss is alive at objectLocationStart %04X (%02X)",
                    //         activeListing.objectLocationStart, aa_genesis_getWorkRam(activeListing.objectLocationStart)
                    //         );
                    //     cartLoader_appendToLog(bossAliveLog);
                    // }
                    // for (int i = 0; i < 0x20; i++) {
                    //     int index = activeListing.additionalHealthByteLocations[i];
                    //     if (index > 0) {
                    //         if (aa_genesis_getWorkRam(index) != 0) {
                    //             bossIsAlive = 1;

                    //             char bossAliveLog[0x100];
                    //             sprintf(bossAliveLog, "----- Sonic 3D: boss is alive at additionalHealthByteLocations[%i] %04X (%02X)",
                    //                 index,
                    //                 activeListing.objectLocationStart, aa_genesis_getWorkRam(activeListing.objectLocationStart)
                    //                 );
                    //             cartLoader_appendToLog(bossAliveLog);
                    //         }
                    //     }
                    // }

                    // char carryLog[0x100];
                    // sprintf(carryLog, "Sonic 3D: is boss alive?? %i",
                    //     bossIsAlive
                    //     );
                    // cartLoader_appendToLog(carryLog);

                    // if (bossIsAlive == 0) {
                    //     // abort early if we've done a level transition!
                    //     return;
                    // }
                }
            } else {
                cartLoader_appendToLog("Caching rings on boss hit");
            }

            bossRushRingCarryValue[0] = 0;
            bossRushRingCarryValue[1] = 0;
            bossRushRingCarryTotal = 0;

            if (gameTransferListing.ringBytesForTransfer[0] > 0) {
                bossRushRingCarryValue[0] = aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[0] % 0x10000);
            }
            if (gameTransferListing.ringBytesForTransfer[1] > 0) {
                bossRushRingCarryValue[1] = aa_genesis_getWorkRam(gameTransferListing.ringBytesForTransfer[1] % 0x10000);
            }

            if (gameTransferListing.ringCalculatationType == 1) {
                int units = bossRushRingCarryValue[0] % 0x10;
                int tens = bossRushRingCarryValue[0] / 0x10;
                int hundreds = bossRushRingCarryValue[1] % 0x10;
                int thousands = bossRushRingCarryValue[1] / 0x10;
                bossRushRingCarryTotal = (1000 * thousands) + (100 * hundreds) + (10 * tens) + units;
            } else {
                bossRushRingCarryTotal = bossRushRingCarryValue[0] + (0x100 * bossRushRingCarryValue[1]);
            }

            char carryLog[0x100];
            sprintf(carryLog, "Carrying %i rings %i %i %i (game %i %i %i)",
                bossRushRingCarryTotal,
                bossRushOptions.carryRingsAcrossGames,
                bossRushOptions.preventCarryInDoomsday,
                getActiveBossRushListing().blockRingZeroing,
                getActiveBossRushListing().gameIndex,
                getActiveBossRushListing().zoneIndex,
                getActiveBossRushListing().actIndex
                );
            cartLoader_appendToLog(carryLog);
        }
    }
}

void bumpToNextBossRush() {
    queueBossRushSlots();
    cacheRingCountInBossRush(0);

    int allowedIndexes[MAX_ROMS];
    int indexesWithoutActivity[MAX_ROMS];
    int maxIndex = 0;
    int maxIndexWithoutActivity = 0;
    for (int i = 0; i < MAX_SIMULTANEOUS_BOSSES; i++) {
        if (i != currentBossRushIndex && activeBossRushes[i] != -1) {
            allowedIndexes[maxIndex] = i;
            maxIndex++;

            if (bossRushCallenges[activeBossRushes[i]].hasBeganPlaying == 0) {
                char tempLog0A[256];
                sprintf(tempLog0A,"bumpToNextBossRush has not got any play for rush %i", activeBossRushes[i]);
                cartLoader_appendToLog(tempLog0A);

                indexesWithoutActivity[maxIndexWithoutActivity] = i;
                maxIndexWithoutActivity++;
            }
        }
    }

    char tempLog1[256];
    sprintf(tempLog1,"bumpToNextBossRush allowedIndexes: %i", maxIndex);
    cartLoader_appendToLog(tempLog1);

    int isUnplayedRush = 0;
    // prefer rushes that haven't been started yet, unless we're on no-switching! (Otherwise the first 3 zones never come up)
    if (maxIndexWithoutActivity > 0 && menuDisplay_getBossRushOptions().switchTrigger != 4) {
        isUnplayedRush = 1;
        for (int i = 0; i < MAX_ROMS; i++) {
            allowedIndexes[i] = indexesWithoutActivity[i];
        }
        isUnplayedRush = 1;
        maxIndex = 1;
        
        char tempLog0[256];
        sprintf(tempLog0,"bumpToNextBossRush flattening to the unplayed %i", maxIndexWithoutActivity);
        cartLoader_appendToLog(tempLog0);
    }

    if (maxIndex > 0) {
        saveActiveBossRushSlot();

        char tempLog2[256];
        sprintf(tempLog2,"bumpToNextBossRush last index: %i", currentBossRushIndex);
        cartLoader_appendToLog(tempLog2);

        // use getNextRandomBossRushNumber ?
        currentBossRushIndex = allowedIndexes[rand() % maxIndex];

        char tempLog3[256];
        sprintf(tempLog3,"bumpToNextBossRush next index: %i", currentBossRushIndex);
        cartLoader_appendToLog(tempLog3);
        
        cartLoader_loadRomAtIndex(getActiveBossRushListing().romAtIndex, 1);

        char tempLog4[256];
        sprintf(tempLog4,"bumpToNextBossRush loaded rom %i", getActiveBossRushListing().romAtIndex);
        cartLoader_appendToLog(tempLog4);

        loadActiveBossRushSlot();

        char tempLog5[256];
        sprintf(tempLog5,"bumpToNextBossRush loaded active boss rush slot");
        cartLoader_appendToLog(tempLog5);

        if (bossRushCallenges[getActiveBossRushIndex()].hasBeganPlaying == 0) {
            // I wanted to set to load specific levels but it doesn't work
            // unsigned int starpostLoc = 0xFE31;
            if (getActiveBossRushListing().gameIndex == 3 || getActiveBossRushListing().gameIndex == 4) {
                aa_genesis_setWorkRam(0xFE2A, getActiveBossRushListing().checkpointIndex);
            }
            // aa_genesis_setWorkRam(0xFE11, getActiveBossRushListing().zonePointer);
            // aa_genesis_setWorkRam(0xFE10, getActiveBossRushListing().actPointer);
            
            // for sonic games, send value 0x8C to location 0xF601 to force a level reset - should fix version clashes!
            if (getActiveBossRushListing().gameIndex <= 4) {
                aa_genesis_setWorkRam(0xF601, 0x8C);
            }
            beginCountdownToApplyBossRushRings();

            for (int i = 0; i < 8; i++) {
                int flag = cheatFlagsPerBossRush[getActiveBossRushIndex()][i];
                if (flag > 0) {
                    aa_genesis_setWorkRam(flag, 0);
                }
            }
        }
        bossRushCallenges[getActiveBossRushIndex()].hasBeganPlaying = 1;

        char tempLog6[256];
        sprintf(tempLog6,"bumpToNextBossRush now playing boss %i, hasBegan %i", getActiveBossRushIndex(), getActiveBossRushListing().hasBeganPlaying);
        cartLoader_appendToLog(tempLog6);

        gameSwapCount++;
    }

    bossRushSwitchCount++;
}

void saveActiveBossRushSlot() {
    if (currentBossRushIndex > -1) {
        saveStateForCurrentBoss();
    }
}

void loadActiveBossRushSlot() {
    if (currentBossRushIndex > -1) {
        loadStateForCurrentBoss();
    }
}

int cartIndexForEachRom[MAX_ROMS];

int getCartIndexForRomAtIndex(int index) {
    return cartIndexForEachRom[index];
}

void mapBossRushesToRoms() {
    for (int i = 0; i < MAX_ROMS; i++) {
        cartIndexForEachRom[i] = -1;
    }

    for (int i = 0; i < romCount; i++) {
        cartLoader_loadRomAtIndex(i, 0);
        int index = cartLoader_getActiveCartIndex();

        char tempLog1[256];
        sprintf(tempLog1,"mapBossRushesToRoms: ROM %i is cart %i", i, index);
        cartLoader_appendToLog(tempLog1);

        for (int brI = 0; brI < bossRushChallengeCount; brI++) {
            if (bossRushCallenges[brI].gameIndex == index && bossRushCallenges[brI].romAtIndex == -1) {
                bossRushCallenges[brI].romAtIndex = i;

                char tempLog[256];
                sprintf(tempLog,"Rush index %i matches rom %i", brI, i);
                cartLoader_appendToLog(tempLog);
            }
        }

        cartIndexForEachRom[i] = index;
    }
}

void resetAllBossRushSlots() {
    for (int i = 0; i < MAX_ROMS; i++) {
        activeBossRushes[i] = -1;
    }
}

void queueBossRushSlots() {
    MAX_SIMULTANEOUS_BOSSES = getMaxSimultaneousBosses();

    // // account for changes in rush count by zeroing anything that's in a slot too high
    for (int i = MAX_SIMULTANEOUS_BOSSES; i < MAX_ROMS; i++) {
        activeBossRushes[i] = -1;
    }

    // then put something in each unoccupied slot
    for (int i = 0; i < MAX_SIMULTANEOUS_BOSSES; i++) {
        if (activeBossRushes[i] == -1) {
            
            queueBossRushInSlot(i);
        }    
    }
}

int getCountOfQueueableRushes() {
    int count = 0;
    for (int i = 0; i < MAX_ROMS; i++) {
        if (challengeCanBeQueued(i)) {
            count++;
        }
    }
    return count;
}

int getCompletedRushCount() {
    int count = 0;
    for (int i = 0; i < bossRushChallengeCount; i++) {
        if (bossRushCallenges[i].isCompleted == 1 && bossRushCallenges[i].romAtIndex != -1 && hasBossRushSaveState[i] == 1) {
            count ++;
        }
    }
    return count;
}

int getEnabledRushCount() {
    int count = 0;
    for (int i = 0; i < bossRushChallengeCount; i++) {
        if (bossRushCallenges[i].romAtIndex != -1 && hasBossRushSaveState[i] == 1) {
            count ++;
        }
    }
    return count;
}

void checkForBossRushComplete() {
    int hasIncompleteRush = 0;
    for (int i = 0; i < bossRushChallengeCount; i++) {
        if (bossRushCallenges[i].isCompleted == 0 && bossRushCallenges[i].romAtIndex != -1 && hasBossRushSaveState[i] == 1) {
            hasIncompleteRush = 1;
        }
    }

    if (hasIncompleteRush == 0) {
        onBossRushComplete();
        bossRushComplete = 1;
    }
}


void onBossRushComplete() {
    // load the credits!
    if (getActiveBossRushListing().gameIndex == 1) {
        // SONIC 1 - end credits
        aa_genesis_setWorkRam(0xF601, 0x9C);
    }
    if (getActiveBossRushListing().gameIndex == 2) {
        // SONIC 2 - you stop in end credits, so go to 2P race over screen
        aa_genesis_setWorkRam(0xF601, 0x98);
    }
    if (getActiveBossRushListing().gameIndex == 3) {
        // SONIC 3 - you stop in end credits, so go to 2P race over screen
        aa_genesis_setWorkRam(0xF601, 0xC4);
    }
    if (getActiveBossRushListing().gameIndex == 4) {
        // SONIC & KNUCKLES - you stop in end credits, so go to get blue spheres
        aa_genesis_setWorkRam(0xF601, 0xAC);
    }
    if (getActiveBossRushListing().gameIndex == 6) {
        // SONIC 3D Blast
        // go to the secret level select?
        aa_genesis_setWorkRam(0x03B7, 0x12);
    }

    flagAllBossRushProgressAsComplete();
}

int challengeCanBeQueued(int i) {
    for (int j = 0; j < MAX_SIMULTANEOUS_BOSSES; j++) {
        if (activeBossRushes[j] == i) {
            return 0;
        }
    }

    if (bossRushCallenges[i].isActivated == 0 && bossRushCallenges[i].isCompleted == 0 && bossRushCallenges[i].romAtIndex != -1 && hasBossRushSaveState[i] == 1) {
        return 1;
    }
    return 0;
}

int numberOfRushesInGame(int gameIdx) {
    int rushCount = 0;
    for (int i = 0; i < bossRushChallengeCount; i++) {
        if (bossRushCallenges[i].gameIndex == gameIdx) {
            rushCount++;
        }
    }
    return rushCount;
}

void queueBossRushInSlot(int slot) {
    int allowedIndexes[MAX_ROMS];
    int maxIndex = 0;


    if (menuDisplay_getBossRushOptions().bossOrder == 0) {
        // pure random order
        for (int i = 0; i < bossRushChallengeCount; i++) {
            if (challengeCanBeQueued(i)) {
                allowedIndexes[maxIndex] = i;
                maxIndex++;
            }
        }

    } else if (menuDisplay_getBossRushOptions().bossOrder == 1) {
        // final boss is always last for each game
        int easiestDifficultyPerGame[MAX_ROMS];
        for (int i = 0; i < MAX_ROMS; i++) {
            easiestDifficultyPerGame[i] = 100;
        }
        for (int i = 0; i < bossRushChallengeCount; i++) {
            if (challengeCanBeQueued(i)) {
                int difficulty = bossRushCallenges[i].shouldAppearInGeneration;

                char tempDiffLogA[256];
                sprintf(tempDiffLogA,"finding easiestDifficultyPerGame: game %i is difficulty %i", i, difficulty);
                cartLoader_appendToLog(tempDiffLogA);

                if (difficulty < easiestDifficultyPerGame[bossRushCallenges[i].gameIndex]) {
                    easiestDifficultyPerGame[bossRushCallenges[i].gameIndex] = difficulty;
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            char tempDiffLog[256];
            sprintf(tempDiffLog,"easiestDifficultyPerGame %i is %i", i, easiestDifficultyPerGame[i]);
            cartLoader_appendToLog(tempDiffLog);
        }


        for (int i = 0; i < bossRushChallengeCount; i++) {
            if (challengeCanBeQueued(i)) {
                if (bossRushCallenges[i].shouldAppearInGeneration == easiestDifficultyPerGame[bossRushCallenges[i].gameIndex]) {
                    allowedIndexes[maxIndex] = i;
                    maxIndex++;
                }
            }
        }
    } else if (menuDisplay_getBossRushOptions().bossOrder == 2) {
        // pure chronological order
        for (int i = 0; i < bossRushChallengeCount; i++) {
            if (challengeCanBeQueued(i)) {
                allowedIndexes[maxIndex] = i;
                maxIndex++;
                break;
            }
        }

    } else if (menuDisplay_getBossRushOptions().bossOrder == 3) {
        // chronological order per game

        int hasFoundFirstPerGame[MAX_ROMS];
        for (int i = 0; i < MAX_ROMS; i++) {
            hasFoundFirstPerGame[i] = 0;
        }
        for (int i = 0; i < bossRushChallengeCount; i++) {
            if (challengeCanBeQueued(i)) {
                int gameIndex = bossRushCallenges[i].gameIndex;
                if (hasFoundFirstPerGame[gameIndex] == 0) {
                    allowedIndexes[maxIndex] = i;
                    maxIndex++;
                    hasFoundFirstPerGame[gameIndex] = 1;
                }
            }
        }

    } else if (menuDisplay_getBossRushOptions().bossOrder == 4) {
        // balanced chronological order per game (more likely to be in a game with more bosses,
        // so that they should all finish at *roughly* the same time)

        int hasFoundFirstPerGame[MAX_ROMS];
        for (int i = 0; i < MAX_ROMS; i++) {
            hasFoundFirstPerGame[i] = 0;
        }
        for (int i = 0; i < bossRushChallengeCount; i++) {
            if (challengeCanBeQueued(i)) {
                int gameIndex = bossRushCallenges[i].gameIndex;
                if (hasFoundFirstPerGame[gameIndex] == 0) {
                    hasFoundFirstPerGame[gameIndex] = 1;
                    int rushCount = numberOfRushesInGame(gameIndex);
                    for (int dupe = 0; dupe < rushCount; dupe++) {
                        allowedIndexes[maxIndex] = i;
                        maxIndex++;
                    }
                }
            }
        }

    }

    if (maxIndex > 0) {
        /*
            SEED THE RANDOM NUUMBER GENERATOR!
        */
        int activeStateSeed = getCountOfQueueableRushes();
        srand(getBossRushSeedWithPrefix(activeStateSeed));

        int chosenIndex = rand() % maxIndex;
        int bossRushIndexToQueue = allowedIndexes[chosenIndex];
        bossRushCallenges[bossRushIndexToQueue].isActivated = 1;
        activeBossRushes[slot] = bossRushIndexToQueue;

        char tempLog2[256];
        sprintf(tempLog2,"    %i <-- %i <-- %i", slot, bossRushIndexToQueue, chosenIndex);
        cartLoader_appendToLog(tempLog2);

        /*
            NOW RESET THE NUUMBER GENERATOR!
        */
        srand(time(NULL));
    } else {
        char tempLog2[256];
        sprintf(tempLog2,"   no valid indexes");
        cartLoader_appendToLog(tempLog2);
    }
}

int getBossRushSeedWithPrefix(int prefix) {
    int menuSeed = getBossRushRandomSeedFromMenu();
    return ((prefix % 0x100) * 0x10000) + menuSeed;
}

int getBossRushRandomSeedFromMenu() {
    return (menuDisplay_getBossRushOptions().orderSeed[0] * 0x1000)
            + (menuDisplay_getBossRushOptions().orderSeed[1] * 0x100)
            + (menuDisplay_getBossRushOptions().orderSeed[2] * 0x10)
            + (menuDisplay_getBossRushOptions().orderSeed[3] * 0x1);
}

void populateBossRushes() {
    // do addBossRushListing for each game
    bossRushChallengeCount = 0;

    // Sonic 1 - https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_(16-bit)/Object_Editing
    int sonic1index = bossRushChallengeCount;
    // GHZ
    addBossRushListing(1, 0, 2, 0xD801, 0xEFFF, 0x40, 0x1F, 0xF7A6, 0x02);
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0xFFE0;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][1] = 0xFFE1;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][2] = 0xFFE2;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][3] = 0xFFE3;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][4] = 0xFFFA;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][5] = 0xFFFB;
    populateMostRecentBossRush4(0x3D, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(0, 2, 0);
    // MZ
    duplicateBossRushListing(sonic1index, 1, 2);
    populateMostRecentBossRush4(0x73, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(2, 2, 0);
    // SYZ
    duplicateBossRushListing(sonic1index, 2, 2);
    populateMostRecentBossRush4(0x75, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(4, 2, 0);
    // LZ
    duplicateBossRushListing(sonic1index, 3, 2);
    populateMostRecentBossRush4(0x77, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(1, 2, 0);
    // SLZ
    duplicateBossRushListing(sonic1index, 4, 2);
    populateMostRecentBossRush4(0x7A, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(3, 2, 0);
    // FinalZone
    duplicateBossRushListing(sonic1index, 5, 2);
    populateMostRecentBossRush4(0x85, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(5, 2, 0);
    applyGenerationToMostRecentBossRush(1); // <-- make it the final challenge in the run
    applyEndValuesToMostRecentBossRush(0xF601, 0x18); // <-- detect the end credits spawning

    // Sonic 2 - https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_2_(16-bit)/Object_Editing/Pointers
    int sonic2index = bossRushChallengeCount;
    addBossRushListing(2, 0, 1, 0xB001, 0xD5FF, 0x40, 0x1F, 0xF7D7, 0x01); // <-- this is the "show countdown" flag - also try F7D2 - F7D5 being non-zero (is it possible to get a zero time bonus?)
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0xFFD0;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][1] = 0xFFD1;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][2] = 0xFFFA;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][3] = 0xFFFB;
    // EHZ
    populateMostRecentBossRush4(0x56, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(0, 1, 0);
    // CPZ
    duplicateBossRushListing(sonic2index, 1, 1);
    populateMostRecentBossRush4(0x5D, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(0x0D, 1, 0);
    // ARZ
    duplicateBossRushListing(sonic2index, 2, 1);
    populateMostRecentBossRush4(0x89, 0, 0, 0);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x32;
    applyZoneLocationValuesToMostRecentBossRush(0x0F, 1, 0);
    // CNZ
    duplicateBossRushListing(sonic2index, 3, 1);
    populateMostRecentBossRush4(0x51, 0, 0, 0);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x32;
    applyZoneLocationValuesToMostRecentBossRush(0x0C, 1, 0);
    // HTZ
    duplicateBossRushListing(sonic2index, 4, 1);
    populateMostRecentBossRush4(0x52, 0, 0, 0);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x32;
    applyZoneLocationValuesToMostRecentBossRush(0x07, 1, 0);
    // MCZ
    duplicateBossRushListing(sonic2index, 5, 1);
    populateMostRecentBossRush4(0x57, 0, 0, 0);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x32;
    applyZoneLocationValuesToMostRecentBossRush(0x0B, 1, 0);
    // OOZ
    duplicateBossRushListing(sonic2index, 6, 1);
    populateMostRecentBossRush4(0x55, 0, 0, 0);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x32;
    applyZoneLocationValuesToMostRecentBossRush(0x0A, 1, 0);
    // MZ
    duplicateBossRushListing(sonic2index, 7, 2);
    populateMostRecentBossRush4(0x54, 0, 0, 0);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x32;
    applyZoneLocationValuesToMostRecentBossRush(0x05, 2, 0);
    // WFZ
    duplicateBossRushListing(sonic2index, 9, 0); // <-- to do: fix the bit where the hit byte goes wild during the explosion!
    populateMostRecentBossRush4(0xC5, 0, 0, 0);
    applyZoneLocationValuesToMostRecentBossRush(0x06, 0, 0);
    applyGenerationToMostRecentBossRush(1); // <-- make it the final challenge in the run
    applyEndValuesToMostRecentBossRush(0xFE11, 0x0E); // <-- detect Death Egg loading

    // DEZ - eggrobo and silver sonic
    addBossRushListing(2, 10, 0, 0xB001, 0xD5FF, 0x40, 0x1F, 0xF7D2, 0x100); 
    // duplicateBossRushListing(sonic2index, 10, 0);
    populateMostRecentBossRush4(0xC7, 0xAF, 0, 0); // C7 is eggrobo, AF is silver sonic
    applyZoneLocationValuesToMostRecentBossRush(0x0E, 0, 0);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x1F;
    applyGenerationToMostRecentBossRush(2); // <-- make it the final challenge in the run
    applyEndValuesToMostRecentBossRush(0xF601, 0x20); // <-- detect the end credits spawning


    // sonic 3 - https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_3_%26_Knuckles/Object_Editing/Pointer_List_1
    int sonic3index = bossRushChallengeCount;
    addBossRushListing(3, 0, 0, 0xB001, 0xCFCB, 0x1, 0x28, 0xF7D2, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    bossRushCallenges[bossRushChallengeCount - 1].objectIdsArePointers = 1;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0xFFD0;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][1] = 0xFFD1;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][2] = 0xFFD2;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][3] = 0xFFD3;
    // AIZ 1
    populateMostRecentBossRush4(0x04, 0x00, 0x74, 0x69); 
    // Are the pointers different on the Sonic 3 alone cartridge? Because this is what comes up in Bizhawk when I search 0x29 bytes before the health value...
    // I think they are, but also the bytes are swapped around compared to bizhawk 0xAB12 in Bizhawk is 0x12AB here

    // AIZ 2 - 0004 711E
    duplicateBossRushListing(sonic3index, 0, 1);
    populateMostRecentBossRush4(0x04, 0x00, 0x1E, 0x71);

    // values picked up from BizHawk
    // my cart is 0x30, 0x30 (v00)
    // HCZ 1 - 0004 7DCA ?? MAYBE??
    duplicateBossRushListing(sonic3index, 1, 0);
    populateMostRecentBossRush4(0x04, 0x00, 0xCA, 0x7D);
    // HCZ 2 - 00 04 8D 3C --> 0x04, 0x00, 0x3C, 0x8D
    duplicateBossRushListing(sonic3index, 1, 1);
    populateMostRecentBossRush4(0x04, 0x00, 0x3C, 0x8D);

    // MGZ 1 - 0005 635E --> 0x05, 0x00, 0x5E, 0x63
    duplicateBossRushListing(sonic3index, 2, 0);
    populateMostRecentBossRush4(0x05, 0x00, 0x5E, 0x63);
    // MGZ 2 - 0004 A0F8
    duplicateBossRushListing(sonic3index, 2, 1);
    populateMostRecentBossRush4(0x04, 0x00, 0xF8, 0xA0);

    // CNZ 1 - 0004 B62A ?? is health offset different for this one??
    duplicateBossRushListing(sonic3index, 3, 0);
    populateMostRecentBossRush4(0x04, 0x00, 0x2A, 0xB6);
    bossRushCallenges[bossRushChallengeCount - 1].healthByteOffsets[0] = 0x44;
    // CNZ 2 - 0004 C002
    duplicateBossRushListing(sonic3index, 3, 1);
    populateMostRecentBossRush4(0x04, 0x00, 0x02, 0xC0);

    // ICZ 1 - 0004 E402
    duplicateBossRushListing(sonic3index, 4, 0);
    populateMostRecentBossRush4(0x04, 0x00, 0x02, 0xE4);
    // ICZ 2 - 0004 ED3C
    duplicateBossRushListing(sonic3index, 4, 1);
    populateMostRecentBossRush4(0x04, 0x00, 0x3C, 0xED);

    // LBZ 1 - 0004 F474
    duplicateBossRushListing(sonic3index, 5, 0);
    populateMostRecentBossRush4(0x04, 0x00, 0x74, 0xF4);

    // LBZ 2 - 0005 0418(pt1), 0004 F9AC I THINK!!(pt2), 0005 0CAA (big arm), ENDING (F600 (maybe F601) == 0x20?)
    duplicateBossRushListing(sonic3index, 5, 1);
    populateMostRecentBossRush8(0x05, 0x00, 0x18, 0x04, 0x04, 0x00, 0xAC, 0xF9);
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[8] = 0x05;
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[9] = 0x00;
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[10] = 0xAA;
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[11] = 0x0C;
    applyGenerationToMostRecentBossRush(1); // <-- make it the final challenge in the run
    applyEndValuesToMostRecentBossRush(0xF601, 0x20); // <-- detect the end credits spawning

    int sonicKindex = bossRushChallengeCount;
    addBossRushListing(4, 0, 0, 0xB001, 0xCFCB, 0x1, 0x28, 0xF7D2, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    bossRushCallenges[bossRushChallengeCount - 1].objectIdsArePointers = 1;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0xFFD0;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][1] = 0xFFD1;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][2] = 0xFFD2;
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][3] = 0xFFD3;
    // MHZ1 - 0007 51CA
    populateMostRecentBossRush4(0x07, 0x00, 0xCA, 0x51);

    // S&K pointers are taken directly from https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_3_%26_Knuckles/Object_Editing/Pointer_List_2 
    // wish them luck!!

    // MHZ2 - 0007 5FD4
    duplicateBossRushListing(sonicKindex, 0, 1);
    populateMostRecentBossRush4(0x07, 0x00, 0xD4, 0x5F);

    // FBZ1 - 0006 EE72
    duplicateBossRushListing(sonicKindex, 1, 0);
    populateMostRecentBossRush4(0x06, 0x00, 0x72, 0xEE);
    // FBZ2 - 0007 063A
    duplicateBossRushListing(sonicKindex, 1, 1);
    populateMostRecentBossRush4(0x07, 0x00, 0x3A, 0x06);

    // SOZ1 - 0007 6A12 <-- wrong pointer and makes no sense anyway - so just don't do anything except on level end
    duplicateBossRushListing(sonicKindex, 2, 0);
    populateMostRecentBossRush4(0x00, 0x00, 0x00, 0x00);
    // SOZ2 - 0007 764E
    duplicateBossRushListing(sonicKindex, 2, 1);
    populateMostRecentBossRush4(0x07, 0x00, 0x4E, 0x76);

    // LRZ1 - worm arms (0007 897A), big hand (0007 8538)
    duplicateBossRushListing(sonicKindex, 3, 0);
    populateMostRecentBossRush8(0x07, 0x00, 0x7A, 0x89, 0x07, 0x00, 0x38, 0x85);
    // LRZ2 - 0007 97FA
    duplicateBossRushListing(sonicKindex, 3, 1);
    populateMostRecentBossRush4(0x07, 0x00, 0xFA, 0x97);

    //HPZ (Knuckles) - 0006 3DE0
    duplicateBossRushListing(sonicKindex, 4, 0);
    populateMostRecentBossRush4(0x06, 0x00, 0xE0, 0x3D);
    applyEndValuesToMostRecentBossRush(0xFE11, 0x0A); // <-- detect sky sanctuary

    // SSZ (Mecha Sonic final) - 0007 B288 
    duplicateBossRushListing(sonicKindex, 5, 0);
    populateMostRecentBossRush4(0x07, 0x00, 0x88, 0xB2);

    // DEZ 1 - 0007 DE6E (central tower), 0007 E0A6 (laser dropper)
    duplicateBossRushListing(sonicKindex, 6, 0);
    populateMostRecentBossRush8(0x07, 0x00, 0x6E, 0xDE, 0x07, 0x00, 0xA6, 0xE0);
    // DEZ 2 - 0007 F0DA
    duplicateBossRushListing(sonicKindex, 6, 1);
    populateMostRecentBossRush4(0x07, 0x00, 0xDA, 0xF0);
    applyEndValuesToMostRecentBossRush(0xFE11, 0x17); // <-- detect DEZ finale

    // DEZ Finale - finger [[not 0008 0CF8]] 0008 0D30, emerald capsule 0008 0542, getaway pod 0008 0160
    duplicateBossRushListing(sonicKindex, 6, 2);
    populateMostRecentBossRush8(0x08, 0x00, 0x30, 0x0D, 0x08, 0x00, 0x42, 0x05); // fingers, emerald, escape (12 slots)
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[8] = 0x08;
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[9] = 0x00;
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[10] = 0x60;
    bossRushCallenges[bossRushChallengeCount - 1].objectIdNumbers[11] = 0x01;
    applyGenerationToMostRecentBossRush(1); // <-- make it the penultimate challenge in the run
    applyEndValuesToMostRecentBossRush(0xFE11, 0x0D); // <-- detect ending

    // Doomsday Zone Finale - 0008 1E3C (shuttle),  0008 17DA (big egg robo)
    duplicateBossRushListing(sonicKindex, 7, 0);
    populateMostRecentBossRush8(0x08, 0x00, 0x3C, 0x1E, 0x08, 0x00, 0xDA, 0x17); // fingers, emerald, escape (12 slots)
    applyGenerationToMostRecentBossRush(2); // <-- make it the final challenge in the run
    applyEndValuesToMostRecentBossRush(0xFE11, 0x0D); // <-- detect ending
    bossRushCallenges[bossRushChallengeCount - 1].blockRingZeroing = 1;

    // Sonic 3D Blast
    addBossRushListing(6, 0, 2, 0x0BA8, 0x0BA8, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select
    addBossRushListing(6, 1, 2, 0x0BA8, 0x0BA8, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select
    addBossRushListing(6, 2, 2, 0x0BA8, 0x0BA8, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select
    addBossRushListing(6, 3, 2, 0x0BA8, 0x0BA8, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select
    addBossRushListing(6, 4, 2, 0x0BA8, 0x0BA8, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select
    addBossRushListing(6, 5, 2, 0x0BA8, 0x0BA8, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select
    // panic puppet is different
    addBossRushListing(6, 6, 2, 0x0B82, 0x0B82, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select
    bossRushCallenges[bossRushChallengeCount - 1].additionalHealthByteLocations[0] = 0x0B94;
    
    addBossRushListing(6, 7, 0, 0x0BA8, 0x0BA8, 0x00, 0x00, 0x0233, 0x100); // <-- this is the "show time countdown" flag - value 0x100 means "look for anything that is non-zero!"
    cheatFlagsPerBossRush[bossRushChallengeCount - 1][0] = 0x040C; // switch off level select


    // Steps:
    //  - get to boss
    //  - find health value on boss (check for 8 hp on Robotnik and 6hp on minibosses)
    //  - open the hex editor
    //  - find the 4 values 0x29 bytes before that value (e.g. https://www.calculator.net/hex-calculator.html?number1=b19b&c2op=-&number2=29&calctype=op&x=93&y=31)
    //  - swap neighbours around and put them above. (e.g. seeing 00 06 91 A8 means to put in 06 00 A8 91)


    // Sonic 3 is going to be nasty https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_3_%26_Knuckles/Object_Editing#Object_Pointers
    // Instead of the ID number, each object has a pointer - e.g. the pointer for AIZ1 boss is 0x68A24
    // So, somewhere in the first 4 bytes, the value 0x00068A24 will be found... I think!
    // So what I need to do is keep scanning for that sequence of bytes and, if I find it, flag that
    // location as the location to start looking from. The value in that pointer slot
    // may change while the object is alive, which is why I need to cache it's location and keep looking!
    //
    // should I write a lua script in bizhawk to check if the value 00068A24 ever appears?
    //
    // In bizhawk I find the value for boss health. Then I go back 29 steps, and I find the 4-byte code above!
    // And it doesn't seem to change! So I just need to check for the 4-byte code instead of the other values

    // during play, when you are in boss rush, switching a game will switch game and then put you in
    // a boss rush listing for that game.
    // Only one listing can be active for a game at a time, unless that feature is toggled off.
    // Use the isActivated value to track which one is currently active (and incomplete)
    // We can optionally make it so that we force final boss to appear last.
    
    // If all listings are complete then we remove this game from the shuffler, then force switch again,
    // unless no games are left in the shuffler!

    // roms will be labelled "bossrush_[game]_[zone]_[act].savestate"
}

void addBossRushListing(int gameIndex, int zoneIndex, int actIndex, unsigned int objectLocationStart, unsigned int objectLocationEnd, unsigned int objectLocationSize, unsigned int healthByteOffset, unsigned int defeatedByte, unsigned int defeatedValue) {
    bossRushCallenges[bossRushChallengeCount].gameIndex = gameIndex;
    bossRushCallenges[bossRushChallengeCount].zoneIndex = zoneIndex;
    bossRushCallenges[bossRushChallengeCount].actIndex = actIndex;

    bossRushCallenges[bossRushChallengeCount].objectLocationStart = objectLocationStart;
    bossRushCallenges[bossRushChallengeCount].objectLocationEnd = objectLocationEnd;
    bossRushCallenges[bossRushChallengeCount].objectLocationSize = objectLocationSize;

    bossRushCallenges[bossRushChallengeCount].defeatedByte = defeatedByte;
    bossRushCallenges[bossRushChallengeCount].defeatedValue = defeatedValue;

    bossRushCallenges[bossRushChallengeCount].isActivated = 0;
    bossRushCallenges[bossRushChallengeCount].isCompleted = 0;
    bossRushCallenges[bossRushChallengeCount].hasBeganPlaying = 0;

    bossRushCallenges[bossRushChallengeCount].shouldAppearInGeneration = 0;
    bossRushCallenges[bossRushChallengeCount].objectIdsArePointers = 0;
    bossRushCallenges[bossRushChallengeCount].blockRingZeroing = 0;
    for (int i = 0; i < 0x20; i++) {
        bossRushCallenges[bossRushChallengeCount].objectIdNumbers[i] = 0;
        bossRushCallenges[bossRushChallengeCount].healthByteOffsets[i] = healthByteOffset;
        bossRushCallenges[bossRushChallengeCount].additionalHealthByteLocations[i] = 0;
    }

    bossRushCallenges[bossRushChallengeCount].romAtIndex = -1;

    bossRushCallenges[bossRushChallengeCount].zonePointer = -1;
    bossRushCallenges[bossRushChallengeCount].actPointer = -1;
    bossRushCallenges[bossRushChallengeCount].checkpointIndex = -1;

    for (int i = 0; i < 8; i++) {
        cheatFlagsPerBossRush[bossRushChallengeCount][i] = 0;
    }

    bossRushChallengeCount++;
}

void duplicateBossRushListing(int listingIndex, int zoneIndex, int actIndex) {
    addBossRushListing(
        bossRushCallenges[listingIndex].gameIndex,
        zoneIndex,
        actIndex,
        bossRushCallenges[listingIndex].objectLocationStart,
        bossRushCallenges[listingIndex].objectLocationEnd,
        bossRushCallenges[listingIndex].objectLocationSize,
        bossRushCallenges[listingIndex].healthByteOffsets[0],
        bossRushCallenges[listingIndex].defeatedByte,
        bossRushCallenges[listingIndex].defeatedValue
    );
    bossRushCallenges[bossRushChallengeCount - 1].objectIdsArePointers = bossRushCallenges[listingIndex].objectIdsArePointers;

    for (int i = 0; i < 8; i++) {
        cheatFlagsPerBossRush[bossRushChallengeCount - 1][i] = cheatFlagsPerBossRush[listingIndex][i];
    }
}

void populateMostRecentBossRush4(unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3) {
    populateBossRushObjectIds4(bossRushChallengeCount - 1, id0, id1, id2, id3);
}

void populateMostRecentBossRush8(unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3, unsigned int id4, unsigned int id5, unsigned int id6, unsigned int id7) {
    populateBossRushObjectIds8(bossRushChallengeCount - 1, id0, id1, id2, id3, id4, id5, id6, id7);
}

void applyGenerationToMostRecentBossRush(unsigned int generation) {
    bossRushCallenges[bossRushChallengeCount - 1].shouldAppearInGeneration = generation;
}

void applyEndValuesToMostRecentBossRush(unsigned int endLoc, unsigned int endVal) {
    bossRushCallenges[bossRushChallengeCount - 1].defeatedByte = endLoc;
    bossRushCallenges[bossRushChallengeCount - 1].defeatedValue = endVal;
}

void applyZoneLocationValuesToMostRecentBossRush(unsigned int zone, unsigned int act, unsigned int checkpoint) {
    bossRushCallenges[bossRushChallengeCount - 1].zonePointer = zone;
    bossRushCallenges[bossRushChallengeCount - 1].actPointer = act;
    bossRushCallenges[bossRushChallengeCount - 1].checkpointIndex = checkpoint;
}


void populateBossRushObjectIds4(int listingIndex, unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3) {
    bossRushCallenges[listingIndex].objectIdNumbers[0] = id0;
    bossRushCallenges[listingIndex].objectIdNumbers[1] = id1;
    bossRushCallenges[listingIndex].objectIdNumbers[2] = id2;
    bossRushCallenges[listingIndex].objectIdNumbers[3] = id3;
}

void populateBossRushObjectIds8(int listingIndex, unsigned int id0, unsigned int id1, unsigned int id2, unsigned int id3, unsigned int id4, unsigned int id5, unsigned int id6, unsigned int id7) {
    bossRushCallenges[listingIndex].objectIdNumbers[0] = id0;
    bossRushCallenges[listingIndex].objectIdNumbers[1] = id1;
    bossRushCallenges[listingIndex].objectIdNumbers[2] = id2;
    bossRushCallenges[listingIndex].objectIdNumbers[3] = id3;
    bossRushCallenges[listingIndex].objectIdNumbers[4] = id4;
    bossRushCallenges[listingIndex].objectIdNumbers[5] = id5;
    bossRushCallenges[listingIndex].objectIdNumbers[6] = id6;
    bossRushCallenges[listingIndex].objectIdNumbers[7] = id7;
}


static int shouldStartBossRush = 0;
static int bossRushIsActive = 0;
static int shouldInitialiseBossRush = 0;

void setStartBossRush(int toValue) {
    shouldStartBossRush = toValue;

    if (shouldStartBossRush != 0) {
        abortAllNinesChallengeSettings();
    }
}

void abortAllBossRushSettings() {
    shouldStartBossRush = 0;
    bossRushIsActive = 0;
    shouldInitialiseBossRush = 0;
}

void toggleStartBossRush() {
    shouldStartBossRush = 1 - shouldStartBossRush;

    if (shouldStartBossRush == 1 && (hasInitialisedBossRush == 0 || shouldResetBossRush == 1)) {
        shouldInitialiseBossRush = 1;
    }
}

int awaitingBossRushStart() {
    return shouldStartBossRush;
}

int checkForBossRushStart() {
    bossRushIsActive = shouldStartBossRush;
    if (shouldInitialiseBossRush == 1 || shouldResetBossRush == 1) {
        // flagNewSavestateLoaded();
        shouldInitialiseBossRush = 0;

        return 1;
    }
    return 0;
}

int shouldUseBossRush() {
    return bossRushIsActive;
}

void zeroAllListings() {
    for (int gameIndex = 0; gameIndex < MAX_ROMS; gameIndex++) {
        gameListings[gameIndex].ringByte = 0;
        gameListings[gameIndex].specialRingByte = 0;
        gameListings[gameIndex].valueWriteDuration = 0;
        gameListings[gameIndex].isISO = 0;
        gameListings[gameIndex].ringSwitchCooldown = 0;
        gameListings[gameIndex].unpauseByte = 0;
        gameListings[gameIndex].postRingEffectCooldown = 0;
        standTriggerListings[gameIndex].standingByte = 0;
        standTriggerListings[gameIndex].standingBit = 0;
        standTriggerListings[gameIndex].standingRequiredValue = 0;
        standTriggerListings[gameIndex].standingCooldown = 0;

        for (int i = 0; i < 8; i++) {
            gameListings[gameIndex].livesBytes[i] = 0;
            gameListings[gameIndex].livesByteDestinations[i] = 0;
            gameListings[gameIndex].timeBytes[i] = 0;
            gameListings[gameIndex].timeByteDestinations[i] = 0;
            gameListings[gameIndex].panicBytes[i] = 0;
            gameListings[gameIndex].panicByteDestinations[i] = 0;
            gameListings[gameIndex].updateHUDFlags[i] = 0;

            gameTransferListings[gameIndex].ringBytesForTransfer[i] = 0;
            gameTransferListings[gameIndex].speedBytesForTransfer[i] = 0;
            gameTransferListings[gameIndex].timeBytesForTransfer[i] = 0;
            gameTransferListings[gameIndex].momentumBytesForTransfer[i] = 0;
            gameTransferListings[gameIndex].scoreBytesForTransfer[i] = 0;

            gameListings[gameIndex].bytesToTestForChange[i] = 0;
        }
        gameTransferListings[gameIndex].ringCalculatationType = 0;

        gameTransferListings[gameIndex].gameStateByte = 0;
        for (int i = 0; i < 0x10; i++) {
            gameTransferListings[gameIndex].gameStatesToBlockScramble[i] = -1;
            gameTransferListings[gameIndex].gameStatesToBlockSwitch[i] = -1;
        }

        momentumControlListings[gameIndex].radius = 0;
        momentumControlListings[gameIndex].xByteStart = 0;
        momentumControlListings[gameIndex].yByteStart = 0;
        momentumControlListings[gameIndex].inertiaByte = 0;
        momentumControlListings[gameIndex].inertiaMin = 0;
        momentumControlListings[gameIndex].inertiaMax = 0;

        for (int i = 0; i < 8; i++) {
            scoreMonitorListings[gameIndex].scoreBytes[i] = 0;
            scoreMonitorListings[gameIndex].scoreBytesP2[i] = 0;
        }
        scoreMonitorListings[gameIndex].calculatationType = 0 ;
        scoreMonitorListings[gameIndex].scoreJumpForTrigger = 0;
        scoreMonitorListings[gameIndex].blockJumpFromZero = 0;
        scoreMonitorListings[gameIndex].allowNegativeChange = 0;
        scoreMonitorListings[gameIndex].allowStackRingInputs = 0;

        levelEditListings[gameIndex].startByte = 0;
        levelEditListings[gameIndex].endByte = 0;

        pixelMonitorListings[gameIndex].enabled = 0;

        for (int i = 0; i < 0x100; i++) {
            pixelStatesPerGame[gameIndex][i] = 0;
            pixelMonitorListings[gameIndex].xCoords[i] = 0;
            pixelMonitorListings[gameIndex].yCoords[i] = 0;
            pixelMonitorListings[gameIndex].allowedColours[i] = 0;

        }

        sprintf(terminalNamePerRom[gameIndex], "UNKNOWN %i", gameIndex);
        sprintf(nameOfTrigger[gameIndex], "");

        musicOverrideListings[gameIndex].shouldEditZ80 = 0;
        musicOverrideListings[gameIndex].byteToCheckForTrackChange = 0;
        musicOverrideListings[gameIndex].valueToWriteIntoTrackChangedSlot = 0;
        musicOverrideListings[gameIndex].byteToWriteToForNoMusic = 0;
        musicOverrideListings[gameIndex].valueToWriteForNoMusic = 0;
        musicOverrideListings[gameIndex].haltMusicCountdown = 0;
        musicOverrideListings[gameIndex].lastTrackChangeValue = 0;
        musicOverrideListings[gameIndex].secondByteToWriteToForNoMusic = 0;
        musicOverrideListings[gameIndex].secondValueToWriteForNoMusic = 0;
        musicOverrideListings[gameIndex].byteStringCheckForTrackChange = 0;
        musicOverrideListings[gameIndex].byteStringLengthToWriteForNoMusic = 0;
        musicOverrideListings[gameIndex].applyChangeDuration = 1;
    }
}

char* getTerminalNameForRom(int romIndex) {
    return terminalNamePerRom[romIndex];
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
void listFiles(const char *path, char prefix[])
{
    char fullPath[0x100];
    sprintf(fullPath, "%s%s", path, prefix);

    char logMsg[0x100];
    sprintf(logMsg, "LIST FILES: %s", fullPath);
    cartLoader_appendToLog(logMsg);

    struct dirent *dp;
    DIR *dir = opendir(fullPath);

    while ((dp = readdir(dir)) != NULL)
    {
        char fileLog[0x100];
        sprintf(fileLog, "%s %d", dp->d_name, dp->d_type);
        cartLoader_appendToLog(fileLog);

        if(dp->d_type == 16384 && dp->d_name[0] != '.') {
            char newPrefix[0x100];
            sprintf(newPrefix, "%s%s/", prefix, dp->d_name);
            listFiles(path, newPrefix);
        } if (pathIsRom(dp->d_name, dp->d_namlen) != 0 && romCount < MAX_ROMS) {
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
            sprintf(romFilePrefixes[romCount], "%s", prefix);
            romCount++;
            cartLoader_appendToLog("new-added rom");
            cartLoader_appendToLog(romFilePrefixes[romCount]);
            cartLoader_appendToLog(romFileNames[romCount]);
        }
        if (pathIsZip(dp->d_name, dp->d_namlen)) {
            foundZipFiles++;
        }
    }

    closedir(dir);
}

int cartLoader_getFoundZipCount() {
    return foundZipFiles;
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

void cartLoader_isolateRomAtIndex(int index) {
    for (int i = 0; i < romCount; i++) {
        if (i == index) {
            romsRemovedFromRandomiser[i] = 0;
        } else {
            romsRemovedFromRandomiser[i] = 1;
        }
    }
}

static int loadAttemptCount = 0;
void cartLoader_loadRandomRom() {
    HackOptions hackOpts = menuDisplay_getHackOptions();
    if (hackOpts.swapOrder == 0) {
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
    } else if (hackOpts.swapOrder == 1) {
        int chosenIndex = lastLoadedIndex;
        for (int offset = 1; offset < romCount; offset++) {
            int candidateIndex = (chosenIndex + offset) % romCount;
            if (romsRemovedFromRandomiser[candidateIndex] == 0) {
                chosenIndex = candidateIndex;
                break;
            }
        }

        if (chosenIndex != lastLoadedIndex) {
            cartLoader_loadRomAtIndex(chosenIndex, 1);
            gameSwapCount++;
        }
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

void cartLoader_setAllGamesAsBlocked() {
    for (int i = 0; i < MAX_ROMS; i++) {
        romsRemovedFromRandomiser[i] = 1;
    }
}

void cartLoader_setAllGamesAsUnblocked() {
    for (int i = 0; i < MAX_ROMS; i++) {
        romsRemovedFromRandomiser[i] = 0;
    }
}

void cartLoader_setRandomSelectionOfGamesAsUnblocked(int maxCount) {
    cartLoader_setAllGamesAsBlocked();
    for (int i = 0 ; i < maxCount; i++) {
        romsRemovedFromRandomiser[rand() % romCount] = 0;
    }

    int totalRandom = 0;
    for (int i = 0; i < romCount; i++) {
        totalRandom += 1 - romsRemovedFromRandomiser[i];
    }

    if (totalRandom < maxCount && maxCount < romCount / 4) {
        cartLoader_setRandomSelectionOfGamesAsUnblocked(maxCount);
    }
}

void cartLoader_blockGamesWithCartNumber(int cartNumber) {
    for (int i = 0; i < MAX_ROMS; i++) {
        if (getCartIndexForRomAtIndex(i) == cartNumber) {
            romsRemovedFromRandomiser[i] = 1;
        }
    }
}


void cartLoader_unblockGamesWithCartNumber(int cartNumber) {
    for (int i = 0; i < MAX_ROMS; i++) {
        if (getCartIndexForRomAtIndex(i) == cartNumber) {
            romsRemovedFromRandomiser[i] = 0;
        }
    }
}

void cartLoader_setGameBlockedAtIndex(int index, int toValue) {
    if (index >= 0 && index < MAX_ROMS) {
        romsRemovedFromRandomiser[index] = 1 - romsRemovedFromRandomiser[index];
    }
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

void cartLoader_getRomFilePrefix(int index, char intoArray[]) {
    for(int i = 0; i < 0x100; i++) {
        intoArray[i] = romFilePrefixes[index][i];
    }
}

void cartLoader_loadRomAtIndex(int index, int shouldCache) {
    int previousConsoleType = cartLoader_consoleForCurrentCart();
    if (cartLoader_consoleForCurrentCart() == 0) {
        hasBeenNonSMS = 1;
    }

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
    sprintf(fullPath, "%s%s%s", folderPath, romFilePrefixes[index], romFileNames[index]);

    cartLoader_appendToLog(fullPath);

    load_rom(fullPath);
    if (fileName256IsCD(romFileNames[index]) == 1) {
        system_hw = SYSTEM_MCD;
        bitmap.viewport.changed = 1;
    }
    if (cartLoader_consoleForCurrentCart() == CART_TYPE_MASTERSYSTEM) {
        system_hw = SYSTEM_SMS;
        bitmap.viewport.changed = 1;
    }
    if (cartLoader_consoleForCurrentCart() == CART_TYPE_GAMEGEAR) {
        system_hw = SYSTEM_GG;
        bitmap.viewport.changed = 1;
    }
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

    // if (hasBeenNonSMS) { //(previousConsoleType != cartLoader_consoleForCurrentCart()) {
    //     if (cartLoader_consoleForCurrentCart() != 0) {
    //         // vdp_setAlistairOffset(64, 16); /// <-- either not working yet or not being called
    //         // replace with a multiply effect?
    //         vdp_setAlistairScale(120, 100);
    //     } else {
    //         // vdp_setAlistairOffset(0, 0);
    //         vdp_setAlistairScale(100, 100);
    //     }
    //     lastSystemType = system_hw;
    // }

    cachedCartIndex = cartLoader_getActiveCartIndex();

    char cartTypeChar = NETWORK_MSG_GAME_IS_GENESIS;
    if (system_hw == SYSTEM_GG || system_hw == SYSTEM_SMS) {
        cartTypeChar = NETWORK_MSG_GAME_IS_MS;
    }
    // Tell the network which game we've switched to
    if (menuDisplay_getNetworkOptions().networkingIsActive != 0) {
        char currentLevelMsg[0x100];
        sprintf(currentLevelMsg, "%c%i%c%c", NETWORK_MSG_SEND_GAME_INDEX_START, cachedCartIndex, NETWORK_MSG_SEND_GAME_INDEX_END, cartTypeChar);
        cartLoader_writeActionToNetwork(currentLevelMsg);
    }

    menuDisplay_generateRulesNameForCurrentGame();

    reportToLED("0");
}

int fileName256IsCD(char fileName[]) {
    for (int i = 0; i < 0x100 - 4; i++) {
        if (fileName[i] == '.' && fileName[i + 1] == 'i' && fileName[i + 2] == 's' && fileName[i + 3] == 'o') {
            return 1;
        }

        if (fileName[i] == 0 || fileName[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

int cartLoader_consoleForCurrentCart() {
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

    // char debugLog[0x100];
    // sprintf(debugLog, "Checking for SMS - comparing ROM HEADERS: %s > %s", checkText, romText);
    // cartLoader_appendToLog(debugLog);

    if (cartLoader_string32AreEqual(checkText, romText) != 0) {
        // cartLoader_appendToLog("It's SMS or GG...");
        unsigned char regionByte = cart.rom[0x7FFF] / 0x10;
        char logText[0x100];
        // sprintf(logText, "Region code: %d", regionByte);
        if (regionByte <= 4) {
            // cartLoader_appendToLog("It's SMS!");
            return CART_TYPE_MASTERSYSTEM;
        } else {
            // cartLoader_appendToLog("It's GG!");
            return CART_TYPE_GAMEGEAR;
        }
    }
    // cartLoader_appendToLog("It's MD");
    return CART_TYPE_MEGADRIVE;
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

    //not found - check it as a byte list (account for japanese games)
    // cartLoader_appendToLog("CHECKING ROM HEADER ALT");
    char altBuffer[0x80];
    sprintf(altBuffer,"%02X%02X%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X%02X%02X",
        romHeaderBuffer[0],
        romHeaderBuffer[1],
        romHeaderBuffer[2],
        romHeaderBuffer[3],
        romHeaderBuffer[4],
        romHeaderBuffer[5],
        romHeaderBuffer[6],
        romHeaderBuffer[7],
        
        romHeaderBuffer[8],
        romHeaderBuffer[9],
        romHeaderBuffer[10],
        romHeaderBuffer[11],
        romHeaderBuffer[12],
        romHeaderBuffer[13],
        romHeaderBuffer[14],
        romHeaderBuffer[15],

        romHeaderBuffer[16],
        romHeaderBuffer[17],
        romHeaderBuffer[18],
        romHeaderBuffer[19],
        romHeaderBuffer[20],
        romHeaderBuffer[21],
        romHeaderBuffer[22],
        romHeaderBuffer[23],

        romHeaderBuffer[24],
        romHeaderBuffer[25],
        romHeaderBuffer[26],
        romHeaderBuffer[27],
        romHeaderBuffer[28],
        romHeaderBuffer[29],
        romHeaderBuffer[30],
        romHeaderBuffer[31]
    );
    // for (int i = 0; i < 0x20; i++) {
    //     char hexValue[2];
    //     sprintf(hexValue, "%02X", romHeaderBuffer[i] % 0x100);
    //     cartLoader_appendToLog(hexValue);
    //     altBuffer[i * 2] = hexValue[0];
    //     altBuffer[(i * 2) + 1] = hexValue[1];
    //     cartLoader_appendToLog(altBuffer);
    // }
    // altBuffer[0x40] = '\0';

    // cartLoader_appendToLog(romHeaderBuffer);
    // cartLoader_appendToLog(altBuffer);

    // for (int i = 1; i < gameListingCount; i++) {
    //     int success = 1;
    //     for (int charIdx = 0; charIdx < 0x40; charIdx++) {
    //         if (altBuffer[charIdx] != gameAltIds[i][charIdx]) {
    //             success = 0;
    //             break;
    //         }
    //     }
    //     if (success == 1) {
    //         return i;
    //     }
    // }

    for (int i = 0; i < gameListingCount; i++) {
        if (modconsole_array32sAreEqual(altBuffer, gameAltIds[i]) == 1) {
            return i;
        }
    }

    // not found - check Master System headers
    modConsole_getMasterSystemProductId(romHeaderBuffer);

    // cartLoader_appendToLog("! No ROM found - checking Master System product ID");
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

AAMusicOverrideListing cartLoader_getActiveMusicOverrideListing() {
    return musicOverrideListings[cartLoader_getActiveCartIndex()];
}

void cartLoader_beginCurrentHaltCountdown() {
    musicOverrideListings[cartLoader_getActiveCartIndex()].haltMusicCountdown = musicOverrideListings[cartLoader_getActiveCartIndex()].applyChangeDuration;
}

void cartLoader_reduceCurrentHaltCountdown() {
    musicOverrideListings[cartLoader_getActiveCartIndex()].haltMusicCountdown--;
}


AAStandTriggerListing cartLoader_getActiveStandTriggerListing() {
    return standTriggerListings[cartLoader_getActiveCartIndex()];
}

AAGameTransferListing cartLoader_getActiveGameTransferListing() {
    return gameTransferListings[cartLoader_getActiveCartIndex()];
}

AAScoreMonitorListing cartLoader_getActiveScoreMonitorListing() {
    return scoreMonitorListings[cartLoader_getActiveCartIndex()];
}

AALevelEditListing cartLoader_getActiveLevelEditListing() {
    return levelEditListings[cartLoader_getActiveCartIndex()];
}

MomentumControlListing cartLoader_getMomentumControlListing() {
    return momentumControlListings[cartLoader_getActiveCartIndex()];
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
        // if (path[pathLen -4] == '.' && path[pathLen - 3] == 'i' && path[pathLen - 2] == 's' && path[pathLen - 1] == 'o' ) {
        //     return 1;
        // }
        if (path[pathLen -3] == '.' && path[pathLen - 2] == 'g' && path[pathLen - 1] == 'g') {
            return 1;
        }
    }
    return 0;
}

int pathIsZip(char *path, int pathLen) {
    if (pathLen > 4) {
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 'z' && path[pathLen - 2] == 'i' && path[pathLen - 1] == 'p' ) {
            return 1;
        }
        if (path[pathLen -3] == '.' && path[pathLen - 2] == '7' && path[pathLen - 1] == 'z') {
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

void cartloader_initialiseRewindDirectory() {
    initialiseDirectory();

    cartLoader_appendToLog("cartloader_initialiseRewindDirectory - BEGINS");

    cartLoader_appendToLog("making directory");

    char command[0x100];
    sprintf(command, "cd %s && mkdir .rewind && cd ..", folderPath);
    cartLoader_appendToLog(command);
    system(command);

    clearRewindDirectory();

    cartLoader_appendToLog("cartloader_initialiseRewindDirectory - DONE");
}


void cartloader_initialiseNetworkDirectories() {
    initialiseDirectory();

    cartLoader_appendToLog("cartloader_initialiseNetworkDirectories - BEGINS");

    cartLoader_appendToLog("making directories");

    char command[0x100];
    sprintf(command, "cd %s && mkdir send && mkdir recv && cd ..", folderPath);
    cartLoader_appendToLog(command);
    system(command);

    clearRecvDirectory();
    clearSendDirectory();

    cartLoader_appendToLog("cartloader_initialiseNetworkDirectories - DONE");
}

void clearSendDirectory() {
    cartLoader_appendToLog("clearSendDirectory");

    char sendPath[0x100];
    sprintf(sendPath, "%s/send/", folderPath);

    struct dirent *dp;
    DIR *dir = opendir(sendPath);

    char filesToRemove[0x1000][0x100];
    int oldFiles = 0;

    while ((dp = readdir(dir)) != NULL)
    {
        if(dp->d_name[0] != '.') {
            char pathThisFile[0x100];
            sprintf(pathThisFile, "%s%s", sendPath, dp->d_name);
            sprintf(filesToRemove[oldFiles], "%s", pathThisFile);
            oldFiles++;
        }
    }
    closedir(dir);

    for (int i = 0; i < oldFiles; i++) {
        char logMessage[0x100];
        sprintf(logMessage, "Deleting from SEND: %s", filesToRemove[i]); 
        cartLoader_appendToLog(logMessage);
        remove(filesToRemove[i]);
    }
}

void clearRecvDirectory() {
    cartLoader_appendToLog("clearRecvDirectory");

    char receivePath[0x100];
    sprintf(receivePath, "%s/recv/", folderPath);

    struct dirent *dp;
    DIR *dir = opendir(receivePath);

    char filesToRemove[0x1000][0x100];
    int oldFiles = 0;

    while ((dp = readdir(dir)) != NULL)
    {
        if(dp->d_name[0] != '.') {
            char pathThisFile[0x100];
            sprintf(pathThisFile, "%s%s", receivePath, dp->d_name);
            sprintf(filesToRemove[oldFiles], "%s", pathThisFile);
            oldFiles++;
        }
    }
    closedir(dir);

    for (int i = 0; i < oldFiles; i++) {
        char logMessage[0x100];
        sprintf(logMessage, "Deleting from SEND: %s", filesToRemove[i]); 
        cartLoader_appendToLog(logMessage);
        remove(filesToRemove[i]);
    }
}

void clearRewindDirectory() {
    cartLoader_appendToLog("clearRewindDirectory");

    char rewindPath[0x100];
    sprintf(rewindPath, "%s/.rewind/", folderPath);

    struct dirent *dp;
    DIR *dir = opendir(rewindPath);

    char filesToRemove[0x1000][0x100];
    int oldFiles = 0;

    while ((dp = readdir(dir)) != NULL)
    {
        if(dp->d_name[0] != '.') {
            char pathThisFile[0x100];
            sprintf(pathThisFile, "%s%s", rewindPath, dp->d_name);
            sprintf(filesToRemove[oldFiles], "%s", pathThisFile);
            oldFiles++;
        }
    }
    closedir(dir);

    for (int i = 0; i < oldFiles; i++) {
        char logMessage[0x100];
        sprintf(logMessage, "Deleting from REWIND: %s", filesToRemove[i]); 
        cartLoader_appendToLog(logMessage);
        remove(filesToRemove[i]);
    }
}

void cartLoader_checkNetworkForActions() {
    char receivePath[0x100];
    sprintf(receivePath, "%s/recv/", folderPath);

    struct dirent *dp;
    DIR *dir = opendir(receivePath);

    char filesToRemove[0x1000][0x100];
    int oldFiles = 0;

    while ((dp = readdir(dir)) != NULL)
    {
        if(dp->d_name[0] != '.') {
            char pathThisFile[0x100];
            sprintf(pathThisFile, "%s%s", receivePath, dp->d_name);
            FILE *reader = fopen(pathThisFile, "r");
            char actionBuffer[0x100];
            sprintf(actionBuffer, "");
            fread(actionBuffer, sizeof(char), 0x100, reader);
            fclose(reader);

            // cartLoader_appendToLog("---ENACING NETWORK EFFECTS");
            // cartLoader_appendToLog(actionBuffer);

            //the below are used for reading "set these settings for the user"
            int interpretType = NETWORK_INTERPRET_TYPE_ACTION;
            int assignNextAsPositive = 0;

            int runningNumber = 0;
            int isFromTwitch = 0;
            int eventLocation = 0;
            int eventDistance = 0;
            int usedNumber = 0;
            int holdDuration = 0;

            for (int i = 0; i < 0x100; i++) {
                char testLog[2];
                testLog[0] = actionBuffer[i];
                testLog[1] = 0;
                cartLoader_appendToLog(testLog);

                if (actionBuffer[i] == 0 || actionBuffer[i] == '!') {
                    cartLoader_appendToLog("---END OF EVENTS");
                    break;
                } else {
                    if (actionBuffer[i] == NETWORK_MSG_INTERPRET_AS_ACTIONS) {
                        interpretType = NETWORK_INTERPRET_TYPE_ACTION;
                    }
                    if (actionBuffer[i] == NETWORK_MSG_INTERPRET_AS_RULES) {
                        interpretType = NETWORK_INTERPRET_TYPE_ASSIGN_RULES;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_INTERPRET_AS_POSITIVE) {
                        assignNextAsPositive = 1;
                    }
                    if (actionBuffer[i] == NETWORK_MSG_INTERPRET_AS_NEGATIVE) {
                        assignNextAsPositive = 0;
                    }
                    if (actionBuffer[i] == NETWORK_MSG_IS_FROM_TWITCH) {
                        isFromTwitch = 1;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_IS_SET_VRAM_STATE) {
                        menudisplay_applyToggleVRAMState(runningNumber);
                    }

                    if (actionBuffer[i] == NETWORK_MSG_REQUEST_RULES) {
                        menuDisplay_sendNetworkOptionsToOpponent();
                        continue;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_USE_ACTIVE_NUM_AS_LOCATION) {
                        eventLocation = runningNumber;
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_USE_ACTIVE_NUM_AS_DISTANCE) {
                        eventDistance = runningNumber;
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_USE_ACTIVE_NUM_AS_HOLD_DURATION) {
                        holdDuration = runningNumber;
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_ENFORCE_SONIC_SPEED) {
                        forceSonicSpeed(runningNumber);
                    }

                    if (actionBuffer[i] == NETWORK_MSG_FIRE_TERMINAL_ACTION) {
                        if (menuDisplay_isShowing() == 0) {
                            if (runningNumber == 1) {
                                modConsole_activatePanic();
                            }
                            if (runningNumber == 2) {
                                modConsole_activateReset();
                            }
                            if (runningNumber == 3) {
                                modConsole_beginRewindAction();
                            }
                            if (runningNumber == 4) {
                                modConsole_endRewindAction();
                            }
                        } else {
                            if (runningNumber != 4 && runningNumber >= 1) {
                                menuDisplay_onButtonPress(INPUT_INDEX_DOWN);
                            }
                        }

                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_ADD_TO_ROTOR_POSITION) {
                        int whichRotor = runningNumber % 8;
                        incrementTerminalRotorValue(whichRotor, 1);
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_REMOVE_FROM_ROTOR_POSITION) {
                        int whichRotor = runningNumber % 8;
                        incrementTerminalRotorValue(whichRotor, -1);
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_HAS_LED_DISPLAY) {
                        setHasLEDDisplay(1);
                    }


                    if (actionBuffer[i] == NETWORK_MSG_START_SPECIFIC_GAME) {
                        cartLoader_loadRomAtIndex(runningNumber, 1);
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_ISOLATE_SPECIFIC_GAME) {
                        cartLoader_isolateRomAtIndex(runningNumber);
                        cartLoader_loadRomAtIndex(runningNumber, 1);
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_ACTIVATE_RULE_PRESET) {
                        menuDisplay_applyPresetRules(runningNumber);
                        runningNumber = 0;
                    }

                    if (actionBuffer[i] == NETWORK_MSG_SHOW_TERMINAL_MENU) {
                        if (menuDisplay_isShowing() == 0) {
                            menuDisplay_showTerminalMenu();
                        } else {
                            menuDisplay_onButtonPress(INPUT_INDEX_START);
                        }
                    }

                    // only interpret actions when the menu is NOT showing!!
                    if (interpretType == NETWORK_INTERPRET_TYPE_ACTION && menuDisplay_isShowing() == 0) {
                        int eventCount = 1;
                        if (usedNumber != 0) {
                            eventCount = runningNumber;
                        }
                        modConsole_processNetworkEvent(actionBuffer[i], eventCount, eventLocation, eventDistance, isFromTwitch, holdDuration);
                    }

                    if (interpretType == NETWORK_INTERPRET_TYPE_ASSIGN_RULES) {
                        menuDisplay_applyNetworkOptionSwitch(actionBuffer[i], assignNextAsPositive);
                    }

                    if (actionBuffer[i] == NETWORK_MSG_REQUEST_RAM_STATE) {

                        int cartSize = 0x10000;
                        if (cartLoader_consoleForCurrentCart() == CART_TYPE_MASTERSYSTEM || cartLoader_consoleForCurrentCart() == CART_TYPE_GAMEGEAR) {
                            cartSize = 0x2000;
                        }
                        int location = runningNumber % cartSize;


                        char logMsg[0x100];
                        sprintf(logMsg, "Attempt to read ram at %04X", location);
                        cartLoader_appendToLog(logMsg);

                        uint8 readValue = aa_genesis_getWorkRam(location);
                        char reportRamState[0x100];
                        sprintf(reportRamState, "RAM:%i=%i", runningNumber, readValue);
                        cartLoader_writeActionToNetwork(reportRamState);

                        char logMsg2[0x100];
                        sprintf(logMsg2, "Reported RAM state %s", reportRamState);
                        cartLoader_appendToLog(logMsg2);
                    }
                }

                if (cartLoader_base10CharToInt(actionBuffer[i]) > -1) {
                    runningNumber *= 10;
                    runningNumber += cartLoader_base10CharToInt(actionBuffer[i]);

                    // flag "a number has been typed!"
                    usedNumber = 1;
                } else {
                    runningNumber = 0;
                }
            }

            sprintf(filesToRemove[oldFiles], "%s", pathThisFile);
            oldFiles++;
        }
    }
    closedir(dir);

    for (int i = 0; i < oldFiles; i++) {
        remove(filesToRemove[i]);
    }
}

void cartLoader_writeActionToNetwork(char action256[]) {
    int eventNumberA = rand() % 0x100;
    int eventNumberB = rand() % 0x100;
    int eventNumberC = rand() % 0x100;
    int eventNumberD = rand() % 0x100;
    int eventNumberE = rand() % 0x100;
    int eventNumberF = rand() % 0x100;
    char eventName[0x100];
    sprintf(eventName, "%02X-%02X-%02X-%02X-%02X-%02X", eventNumberA, eventNumberB, eventNumberC, eventNumberD, eventNumberE, eventNumberF);
    char sendPath[0x100];
    sprintf(sendPath, "%s/send/%s.amb", folderPath, eventName);

    FILE *writer = fopen(sendPath, "w");
    fprintf(writer, action256);
    fclose(writer);
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
    // I'll need to do fclose if I want to see the changes during play
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
    gameListings[toGame].isISO = gameListings[fromGame].isISO;
    gameListings[toGame].ringSwitchCooldown = gameListings[fromGame].ringSwitchCooldown;
    gameListings[toGame].accelerationType = gameListings[fromGame].accelerationType;
    gameListings[toGame].unpauseByte = gameListings[fromGame].unpauseByte;
    gameListings[toGame].postRingEffectCooldown = gameListings[fromGame].postRingEffectCooldown;
    standTriggerListings[toGame].standingBit = standTriggerListings[fromGame].standingBit;
    standTriggerListings[toGame].standingByte = standTriggerListings[fromGame].standingByte;
    standTriggerListings[toGame].standingRequiredValue = standTriggerListings[fromGame].standingRequiredValue;
    standTriggerListings[toGame].standingCooldown = standTriggerListings[fromGame].standingCooldown;

    for (int i = 0; i < 8; i++) {
        gameListings[toGame].livesBytes[i] = gameListings[fromGame].livesBytes[i];
        gameListings[toGame].livesByteDestinations[i] = gameListings[fromGame].livesByteDestinations[i];
        gameListings[toGame].timeBytes[i] = gameListings[fromGame].timeBytes[i];
        gameListings[toGame].timeByteDestinations[i] = gameListings[fromGame].timeByteDestinations[i];
        gameListings[toGame].panicBytes[i] = gameListings[fromGame].panicBytes[i];
        gameListings[toGame].panicByteDestinations[i] = gameListings[fromGame].panicByteDestinations[i];
        gameListings[toGame].updateHUDFlags[i] = gameListings[fromGame].updateHUDFlags[i];

        gameListings[toGame].bytesToTestForChange[i] = gameListings[fromGame].bytesToTestForChange[i];

        gameTransferListings[toGame].ringBytesForTransfer[i] = gameTransferListings[fromGame].ringBytesForTransfer[i];
        gameTransferListings[toGame].speedBytesForTransfer[i] = gameTransferListings[fromGame].speedBytesForTransfer[i];
        gameTransferListings[toGame].timeBytesForTransfer[i] = gameTransferListings[fromGame].timeBytesForTransfer[i];
        gameTransferListings[toGame].momentumBytesForTransfer[i] = gameTransferListings[fromGame].momentumBytesForTransfer[i];
        gameTransferListings[toGame].scoreBytesForTransfer[i] = gameTransferListings[fromGame].scoreBytesForTransfer[i];
    }
    gameTransferListings[toGame].ringCalculatationType = gameTransferListings[fromGame].ringCalculatationType;

    gameTransferListings[toGame].gameStateByte = gameTransferListings[fromGame].gameStateByte;
    for (int i = 0; i < 0x10; i++) {
        gameTransferListings[toGame].gameStatesToBlockScramble[i] = gameTransferListings[fromGame].gameStatesToBlockScramble[i];
        gameTransferListings[toGame].gameStatesToBlockSwitch[i] = gameTransferListings[fromGame].gameStatesToBlockSwitch[i];
    }

    momentumControlListings[toGame].radius = momentumControlListings[fromGame].radius;
    momentumControlListings[toGame].xByteStart = momentumControlListings[fromGame].xByteStart;
    momentumControlListings[toGame].yByteStart = momentumControlListings[fromGame].yByteStart;
    momentumControlListings[toGame].inertiaByte = momentumControlListings[fromGame].inertiaByte;
    momentumControlListings[toGame].inertiaMin = momentumControlListings[fromGame].inertiaMin;
    momentumControlListings[toGame].inertiaMax = momentumControlListings[fromGame].inertiaMax;

    for (int i = 0; i < 8; i++) {
        scoreMonitorListings[toGame].scoreBytes[i] = scoreMonitorListings[fromGame].scoreBytes[i];
        scoreMonitorListings[toGame].scoreBytesP2[i] = scoreMonitorListings[fromGame].scoreBytesP2[i];
    }
    scoreMonitorListings[toGame].calculatationType = scoreMonitorListings[fromGame].calculatationType ;
    scoreMonitorListings[toGame].scoreJumpForTrigger = scoreMonitorListings[fromGame].scoreJumpForTrigger;
    scoreMonitorListings[toGame].blockJumpFromZero = scoreMonitorListings[fromGame].blockJumpFromZero;
    scoreMonitorListings[toGame].allowNegativeChange = scoreMonitorListings[fromGame].allowNegativeChange;
    scoreMonitorListings[toGame].allowStackRingInputs = scoreMonitorListings[fromGame].allowStackRingInputs;

    levelEditListings[toGame].startByte = levelEditListings[fromGame].startByte;
    levelEditListings[toGame].endByte = levelEditListings[fromGame].endByte;

    pixelMonitorListings[toGame].enabled = pixelMonitorListings[fromGame].enabled;
    for (int i = 0; i < 0x100; i++) {
        pixelMonitorListings[toGame].xCoords[i] = pixelMonitorListings[fromGame].xCoords[i];
        pixelMonitorListings[toGame].yCoords[i] = pixelMonitorListings[fromGame].yCoords[i];
        pixelMonitorListings[toGame].allowedColours[i] = pixelMonitorListings[fromGame].allowedColours[i];

    }
    pixelMonitorListings[toGame].changeMustAffectColour = pixelMonitorListings[fromGame].changeMustAffectColour;

    musicOverrideListings[toGame].shouldEditZ80 = musicOverrideListings[fromGame].shouldEditZ80;
    musicOverrideListings[toGame].byteToCheckForTrackChange = musicOverrideListings[fromGame].byteToCheckForTrackChange;
    musicOverrideListings[toGame].valueToWriteIntoTrackChangedSlot = musicOverrideListings[fromGame].valueToWriteIntoTrackChangedSlot;
    musicOverrideListings[toGame].byteToWriteToForNoMusic = musicOverrideListings[fromGame].byteToWriteToForNoMusic;
    musicOverrideListings[toGame].valueToWriteForNoMusic = musicOverrideListings[fromGame].valueToWriteForNoMusic;
    musicOverrideListings[toGame].lastTrackChangeValue = musicOverrideListings[fromGame].lastTrackChangeValue;
    musicOverrideListings[toGame].haltMusicCountdown = musicOverrideListings[fromGame].haltMusicCountdown;
    musicOverrideListings[toGame].secondByteToWriteToForNoMusic = musicOverrideListings[fromGame].secondByteToWriteToForNoMusic;
    musicOverrideListings[toGame].secondValueToWriteForNoMusic = musicOverrideListings[fromGame].secondValueToWriteForNoMusic;
    musicOverrideListings[toGame].byteStringCheckForTrackChange = musicOverrideListings[fromGame].byteStringCheckForTrackChange;
    musicOverrideListings[toGame].applyChangeDuration = musicOverrideListings[fromGame].applyChangeDuration;
    musicOverrideListings[toGame].byteStringLengthToWriteForNoMusic = musicOverrideListings[fromGame].byteStringLengthToWriteForNoMusic;

    sprintf(nameOfTrigger[toGame], nameOfTrigger[fromGame]);
}

void saveStateForCurrentBoss() {
    state_save(bossRushSaveStates[getActiveBossRushIndex()]);
    hasBossRushSaveState[getActiveBossRushIndex()] = 1;
}

void loadStateForCurrentBoss() {
    if (hasBossRushSaveState[getActiveBossRushIndex()] == 0) {
        // load it from the .boss_rush_source folder

        char tempLog1[256];
        sprintf(tempLog1,"loadStateForCurrentBoss has no state at index %i", getActiveBossRushIndex());
        cartLoader_appendToLog(tempLog1);

        return;
    }

    // char tempLog[256];
    // sprintf(tempLog,"Loading save state %d (%s) %d", lastLoadedIndex, loadedRomName, hasCachedSaveState[lastLoadedIndex]);
    // cartLoader_appendToLog(tempLog);

    char tempLog2[256];
    sprintf(tempLog2,"loadStateForCurrentBoss loading boss state at index %i", getActiveBossRushIndex());
    cartLoader_appendToLog(tempLog2);

    state_load(bossRushSaveStates[getActiveBossRushIndex()]);
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
        // In terminal mode, always start from
        // a title screen state if we can
        if (terminalRulesAreActive() != 0 && getIsIdleModeActive() == 0) {
            cartLoader_loadCurrentStartupStateFromDisk();
        }  
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

void cartLoader_saveRewindStateForCurrentGame() {
    // cartLoader_appendToLog("cartLoader_saveRewindStateForCurrentGame");

    // get the current save state
    uint8 saveState[STATE_SIZE];
    state_save(saveState);

    int stepIndex = rewindStateCounterPerGame[lastLoadedIndex];
    char path[256];
    sprintf(path, "%s/.rewind/%i_%i.savestate", folderPath, lastLoadedIndex, stepIndex);
    
    // char tempLog[256];
    // sprintf(tempLog,"Saving rewind save state (game %i, step %i)", lastLoadedIndex, stepIndex);
    // cartLoader_appendToLog(tempLog);
    // cartLoader_appendToLog(path);

    FILE *f = fopen(path,"wb");
    if (f)
    {
        fwrite(&saveState, STATE_SIZE, 1, f);
        fclose(f);
        // cartLoader_appendToLog("success!");
        rewindStateCounterPerGame[lastLoadedIndex]++;
        if (rewindStateMinimumPerGame[lastLoadedIndex] < rewindStateCounterPerGame[lastLoadedIndex] - maxRewindStatesPerGame) {
            deleteRewindState(lastLoadedIndex, rewindStateMinimumPerGame[lastLoadedIndex]);
            rewindStateMinimumPerGame[lastLoadedIndex] = rewindStateCounterPerGame[lastLoadedIndex] - maxRewindStatesPerGame;
        }
    } else {
        // cartLoader_appendToLog("no state found");
    }
}

void deleteRewindState(int gameIndex, int stateIndex) {
    char path[256];
    sprintf(path, "%s/.rewind/%i_%i.savestate", folderPath, gameIndex, stateIndex);

    // char tempLog[256];
    // sprintf(tempLog,"Deleting rewind save state (game %i, step %i)", gameIndex, stateIndex);
    // cartLoader_appendToLog(tempLog);
    // cartLoader_appendToLog(path);

    remove(path);
}

int cartLoader_loadRewindStateForCurrentGame() {
    cartLoader_appendToLog("cartLoader_loadRewindStateForCurrentGame");

    int success = 0;

    if (rewindStateCounterPerGame[lastLoadedIndex] > rewindStateMinimumPerGame[lastLoadedIndex]) {
        int previousStep = rewindStateCounterPerGame[lastLoadedIndex] - 1;

        char path[256];
        sprintf(path, "%s/.rewind/%i_%i.savestate", folderPath, lastLoadedIndex, previousStep);

        char tempLog[256];
        sprintf(tempLog,"Loading rewind save state (game %i, step %i)", lastLoadedIndex, previousStep);
        cartLoader_appendToLog(tempLog);
        cartLoader_appendToLog(path);

        uint8 saveState[STATE_SIZE];

        FILE *f = fopen(path,"rb");
        if (f)
        {
            fread(&saveState, STATE_SIZE, 1, f);
            fclose(f);
            cartLoader_appendToLog("success!");

            rewindStateCounterPerGame[lastLoadedIndex] = previousStep;
            state_load(saveState);
            // and copy this to the pause screen cache just in case!
            state_save(saveStateBeforeMenu);

            success = 1;
        } else {
            cartLoader_appendToLog("no state found");
        }
    }
    return success;
}

void cartLoader_saveAllSaveStatesToDisk() {
    cartLoader_appendToLog("cartLoader_saveAllSaveStatesToDisk");

    for (int i = 0; i < romCount; i++) {
        if (hasCachedSaveState[i] != 0) {
            char path[256];
            sprintf(path, "%s%s_%s.savestate", folderPath, romFilePrefixes[i], romFileNames[i]);
            
            char tempLog[256];
            sprintf(tempLog,"Saving save state %d", i);
            cartLoader_appendToLog(tempLog);
            cartLoader_appendToLog(path);

            FILE *f = fopen(path,"wb");
            if (f)
            {
                fwrite(&cachedSaveStates[i], STATE_SIZE, 1, f);
                fclose(f);
                cartLoader_appendToLog("success!");
            } else {
                cartLoader_appendToLog("not found - could not load");
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
        sprintf(path, "%s%s_%s.savestate", folderPath, romFilePrefixes[i], romFileNames[i]);

        char tempLog[256];
        sprintf(tempLog,"Loading save state %d", i);
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
            cartLoader_appendToLog("no state found");
        }
        cartLoader_appendToLog(" -- ");
    }
}

void cartLoader_loadCurrentStartupStateFromDisk() {
    char path[256];
    sprintf(path, "%s/.startup_states%s_%s.savestate", folderPath, romFilePrefixes[lastLoadedIndex], romFileNames[lastLoadedIndex]);

    char tempLog[256];
    sprintf(tempLog,"Loading STARTUP save state %d", lastLoadedIndex);
    cartLoader_appendToLog(tempLog);
    cartLoader_appendToLog(path);
    
    FILE *f = fopen(path,"rb");
    if (f)
    {
        uint8 saveStateOnDisk[STATE_SIZE];
        fread(&saveStateOnDisk, STATE_SIZE, 1, f);
        fclose(f);
        cartLoader_appendToLog("successfully loaded startup state!");
        state_load(saveStateOnDisk);

    } else {
        cartLoader_appendToLog("no startup state found");
    }
    cartLoader_appendToLog(" -- ");
}

void cartLoader_clearSaveStates() {
    for (int i = 0; i < MAX_ROMS; i++) {
        hasCachedSaveState[i] = 0;
    }
}

void cartLoader_loadBossRushSaveStatesFromDisk() {
    for (int i = 0; i < bossRushChallengeCount; i++) {
        char path[256];
        BossRushChallengeListing listing = bossRushCallenges[i];

        char silentModeFlag[16];
        sprintf(silentModeFlag, "");
        if (menuDisplay_getBossRushOptions().shouldUseExternalMusic) {
        sprintf(silentModeFlag, "/silent");
        }

        // sprintf(path, "%s/0_0_2.savestate", folderPath); // 
        sprintf(path, "%s/.boss_rush_source%s/%i_%i_%i.savestate", folderPath, silentModeFlag, listing.gameIndex, listing.zoneIndex, listing.actIndex);
        // sprintf(path, "%s/.boss_rush_source/%i.savestate", folderPath, listing.gameIndex);

        // LOGGING HERE SEEMS TO CRASH THE EMU!!!
        char tempLog[256];
        sprintf(tempLog,"Loading boss rush state %d", i);
        cartLoader_appendToLog(tempLog);
        cartLoader_appendToLog(path);
        
        FILE *f = fopen(path,"rb");
        if (f)
        {
            fread(&bossRushSaveStates[i], STATE_SIZE, 1, f);
            fclose(f);
            cartLoader_appendToLog("success!");
            hasBossRushSaveState[i] = 1;
        } else {
            cartLoader_appendToLog("no state found");
        }
        cartLoader_appendToLog(" -- ");
    }
}

void cartLoader_applyHackOptions(int gameHasStarted) {
    if (menuDisplay_getHackOptions().loadFromSavedState && gameHasStarted == 0 && shouldUseBossRush() == 0) {
        cartLoader_loadAllSaveStatesFromDisk();
    }
}

static PersistValuesData cachedPersistValues;

void cacheDataToCarryOver() {
    PersistValuesOptions options = menuDisplay_getPersistValuesOptions();
    AAGameListing gameListing = cartLoader_getActiveGameListing();
    AAGameTransferListing gameTransferListing = cartLoader_getActiveGameTransferListing();
    BossRushOptions bossRushOptions = menuDisplay_getBossRushOptions();

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
        
    applyBossRushCachedRings();

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

void writeShortenedFileName(char source256[], char output256[], int length) {
    char beforeDecimal[0x100];
    char afterDecimal[0x100];
    int hasHitDecimal = 0;
    int indexAfterDecimal = 0;
    int lengthBeforeDecimal = 0;

    int decimalIndex = 0;
    for (int i = 0; i < 0x100; i++) {
        if (source256[i] == '.') {
            decimalIndex = i;
        }
        if (source256[i] == 0 || source256[i] == '\0') {
            break;
        }
    }

    for (int i = 0; i < 0x100; i++) {
        if (hasHitDecimal == 0) {
            if (i == decimalIndex) {
                hasHitDecimal = 1;
            } else {
                beforeDecimal[i] = source256[i];
                lengthBeforeDecimal++;
            }
        } else {
            afterDecimal[indexAfterDecimal] = source256[i];
            afterDecimal[indexAfterDecimal + 1] = '\0';
            indexAfterDecimal++;
            if (indexAfterDecimal >= 3) {
                break;
            }
        }
    }

    if (lengthBeforeDecimal + 4 <= length) {
        sprintf(output256, "%s", source256);
        return;
    } else {
        char buffer[0x100];
        for (int i = 0; i < 0x100; i++) {
            buffer[i] = 0;
        } 
        for (int i = 0; i < length - 4; i++) {
            buffer[i] = beforeDecimal[i];
            buffer[i + 1] = '\0';
        }
        sprintf(output256, "%s.%s", buffer, afterDecimal);
    }
}

void cartLoader_updatePixelTracker(int line, unsigned char linebuf[2][0x200]) {
    if (pixelMonitorListings[cachedCartIndex].enabled == 0) {
        return;
    }

    for (int i = 0; i < 0x100; i++) {
        int xPos = pixelMonitorListings[cachedCartIndex].xCoords[i];
        int yPos = pixelMonitorListings[cachedCartIndex].yCoords[i];

        if (xPos > 0 || yPos > 0) {
            if (yPos == line) {
                pixelStatesPerGame[cachedCartIndex][i] = linebuf[0][0x20 + xPos];
            }
        } else {
            break;
        }
    }
}


void cartLoader_checkPixelTrackerForStateChange() {
    if (pixelMonitorListings[cachedCartIndex].enabled == 0) {
        return;
    }

    int allValuesOkay = 1;
    int aValueHasChanged = 0;

    int targetColourWasChanged = 0;
    if (pixelMonitorListings[cachedCartIndex].changeMustAffectColour == 0) {
        targetColourWasChanged = 1;
    }

    for (int i = 0; i < 0x100; i++) {
        int xPos = pixelMonitorListings[cachedCartIndex].xCoords[i];
        int yPos = pixelMonitorListings[cachedCartIndex].yCoords[i];

        if (xPos > 0 || yPos > 0) {
            if (pixelStatesPerGame[cachedCartIndex][i] != lastPixelStatesPerGame[cachedCartIndex][i]) {
                aValueHasChanged = 1;
                int thisPixelAllowed = 0;
                for (int j = 0; j < 0x100; j++) {
                    if (pixelMonitorListings[cachedCartIndex].allowedColours[j] == 0) {
                        break;
                    }
                    if (pixelStatesPerGame[cachedCartIndex][i] == pixelMonitorListings[cachedCartIndex].allowedColours[j]) {
                        thisPixelAllowed = 1;
                    }
                }

                if (pixelStatesPerGame[cachedCartIndex][i] == pixelMonitorListings[cachedCartIndex].changeMustAffectColour) {
                    targetColourWasChanged = 1;
                }

                if (thisPixelAllowed == 0) {
                    allValuesOkay = 0;
                    break;
                }
            }
        } else {
            break;
        }
    }

    if (allValuesOkay == 1 && aValueHasChanged == 1) {
        for (int i = 0; i < 0x100; i++) {
            lastPixelStatesPerGame[cachedCartIndex][i] = pixelStatesPerGame[cachedCartIndex][i];
        }

        if (targetColourWasChanged) {
            if (cartLoader_getActiveGameListing().ringSwitchCooldown > 0) {
                modConsole_setCountdownUntilRingSwitch(cartLoader_getActiveGameListing().ringSwitchCooldown);
            } else {
                promptSwitchGame();
            }
        }
    }
}


static int shouldStartNinesChallenge = 0;
static int ninesChallengeIsActive = 0;
static int shouldInitialiseNinesChallenge = 0;

static int ninesChallengeComplete = 0;

static int shouldResetNinesChallenge = 0;
static int hasInitialisedNinesChallenge = 0;

void setStartNinesChallenge(int toValue) {
    shouldStartNinesChallenge = toValue;

    if (shouldResetNinesChallenge != 0) {
        abortAllBossRushSettings();
    }
}

void abortAllNinesChallengeSettings() {
    shouldStartNinesChallenge = 0;
    ninesChallengeIsActive = 0;
    shouldInitialiseNinesChallenge = 0;
}

void toggleStartNinesChallenge() {
    shouldStartNinesChallenge = 1 - shouldStartNinesChallenge;

    if (shouldStartNinesChallenge == 1 && (hasInitialisedNinesChallenge == 0 || shouldResetNinesChallenge == 1)) {
        shouldInitialiseNinesChallenge = 1;
    }
}


int awaitingNinesChallengeStart() {
    return shouldInitialiseNinesChallenge;
}

int getNinesChallengeComplete() {
    return ninesChallengeComplete;
}

void setShouldResetNinesChallenge(int val) {
    shouldStartNinesChallenge = val;
}

int getShouldShowNinesChallengeAsReadyToReset() {
    if (shouldResetNinesChallenge == 1) {
        return 1;
    }
    if (hasInitialisedNinesChallenge == 0) {
        return 1;
    }
    return 0;
}

int getShouldResetNinesChallenge() {
    return shouldResetNinesChallenge;
}