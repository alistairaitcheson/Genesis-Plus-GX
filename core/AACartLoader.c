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
static char romFileNames[MAX_ROMS][0x100];
static char romFilePrefixes[MAX_ROMS][0x100];
static int romsRemovedFromRandomiser[MAX_ROMS];

static char *logLines[0x100];
static unsigned int logLineCount;

static AAGameListing gameListings[MAX_ROMS];
static AAGameTransferListing gameTransferListings[MAX_ROMS];
static AAScoreMonitorListing scoreMonitorListings[MAX_ROMS];
static AALevelEditListing levelEditListings[MAX_ROMS];
static AAPixelMonitorListing pixelMonitorListings[MAX_ROMS];
static unsigned char gameAltIds[MAX_ROMS][0x80];
static int gameListingCount = 0;

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
    levelEditListings[1].startByte = 0xA408;
    levelEditListings[1].endByte = 0xA800;
    gameListings[1].unpauseByte = 0xF604; //0xF63A;
    gameListings[1].unpauseByteDestination = 0x80; // 0x1;

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
    levelEditListings[2].startByte = 0x8008;
    levelEditListings[2].endByte = 0x9000;

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
    levelEditListings[3].startByte = 0x8008;
    levelEditListings[3].endByte = 0x9000;

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
    gameListings[7].valueWriteDuration = 300; // only update lives every 20 seconds
    
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

    writeStringToArray32("73250", gameListings[11].gameId); // Sonic Blast MS <-- still need to find lives and time
    gameListings[11].ringByte = 0x125E; // <-- WARNING! it is used in the title sequence (once per frame?)
    gameListings[11].specialRingByte =  0x1D9E;
    gameListings[11].livesBytes[0] = 0x1178;
    gameListings[11].livesByteDestinations[0] = 0x5; 
    gameListings[11].timeBytes[0] = 0;
    gameListings[11].timeByteDestinations[0] = 1;
    gameListings[11].valueWriteDuration = 0;
    gameListings[11].valueWriteDuration = 60;
    gameListings[11].ringSwitchCooldown = 8;

    writeStringToArray32("07250", gameListings[12].gameId); // Sonic 2 GG
    gameListings[12].ringByte = 0x1299;
    gameListings[12].specialRingByte = 0;
    gameListings[12].livesBytes[0] = 0x1298;
    gameListings[12].livesByteDestinations[0] = 0x5; 
    gameListings[12].timeBytes[0] = 0x12BA;
    gameListings[12].timeByteDestinations[0] = 1;
    gameListings[12].valueWriteDuration = 0;
    gameListings[12].valueWriteDuration = 60;    

    writeStringToArray32("08240", gameListings[13].gameId); // Sonic 1 GG
    gameListings[13].ringByte = 0x12A9;
    gameListings[13].specialRingByte = 0;
    gameListings[13].livesBytes[0] = 0x1240;
    gameListings[13].livesByteDestinations[0] = 0x5; 
    gameListings[13].timeBytes[0] = 0x12CF;
    gameListings[13].timeByteDestinations[0] = 1;
    gameListings[13].valueWriteDuration = 0;
    gameListings[13].valueWriteDuration = 60;   

    writeStringToArray32("15250", gameListings[14].gameId); // Sonic Chaos GG
    gameListings[14].ringByte = 0x129C;
    gameListings[14].specialRingByte = 0;
    gameListings[14].livesBytes[0] = 0x129B;
    gameListings[14].livesByteDestinations[0] = 0x5; 
    gameListings[14].timeBytes[0] = 0x12C2;
    gameListings[14].timeByteDestinations[0] = 1;
    gameListings[14].valueWriteDuration = 0;
    gameListings[14].valueWriteDuration = 60;   

    writeStringToArray32("73250", gameListings[15].gameId); // Sonic Blast GG <-- still need to find lives and time (identical to SMS)
    copyGameListing(11, 15);

    writeStringToArray32("30250", gameListings[16].gameId); // Sonic Triple Trouble<-- still need to find lives and time
    gameListings[16].ringByte = 0x1159;
    gameListings[16].specialRingByte = 0;
    gameListings[16].livesBytes[0] = 0x1140;
    gameListings[16].livesByteDestinations[0] = 0x5; 
    gameListings[16].timeBytes[0] = 0x115E;
    gameListings[16].timeByteDestinations[0] = 1;
    gameListings[16].valueWriteDuration = 60;   

    
    writeStringToArray32("SONICCD", gameListings[17].gameId); // <-- not used yet
    gameListings[17].ringByte = 0;
    gameListings[17].specialRingByte = 0;
    gameListings[17].livesBytes[0] = 0;
    gameListings[17].livesByteDestinations[0] = 0x5; 
    gameListings[17].timeBytes[0] = 0;
    gameListings[17].timeByteDestinations[0] = 1;
    gameListings[17].valueWriteDuration = 0;
    gameListings[17].valueWriteDuration = 60;   
    gameListings[17].isISO = 1;  // <-- I plan to use this as a way to detect CD games for the time being...
    
    writeStringToArray32("Dr.Robotnik'sMeanBeanMachine", gameListings[18].gameId); // <-- not used yet
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
    gameListings[18].ringSwitchCooldown = 2;

    writeStringToArray32("Puyo Puyo (JP)", gameListings[19].gameId); // <-- puyo puyo
    copyGameListing(18, 19);
    sprintf(gameAltIds[19], "82D582E682D582E6 0000000000000000 0000000000000000 0000000000000000");
    gameListings[19].ringByte = 0;
    gameListings[19].specialRingByte = 0;

    writeStringToArray32("Puyo Puyo 2 (JP)", gameListings[20].gameId); // <-- puyo puyo 2
    sprintf(gameAltIds[20], "82D582E682D582E6 8251000000000000 0000000000000000 0000000000000000");
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
    gameListings[20].ringSwitchCooldown = 2;

    writeStringToArray32("BAREKNUCKLE", gameListings[21].gameId); // <-- Streets of Rage 1
    gameListings[21].ringByte = 0;
    gameListings[21].specialRingByte = 0;
    gameListings[21].livesBytes[0] = 0xFF21;
    gameListings[21].livesByteDestinations[0] = 0x5; 
    gameListings[21].timeBytes[0] = 0xFB00;
    gameListings[21].timeByteDestinations[0] = 50;
    gameListings[21].valueWriteDuration = 0;
    scoreMonitorListings[21].scoreBytes[0] = 0xFF0B; // have to use score because SoR 1 doesn't show enemy health
    scoreMonitorListings[21].scoreBytes[1] = 0xFF08;
    scoreMonitorListings[21].scoreBytesP2[0] = 0;
    scoreMonitorListings[21].scoreBytesP2[1] = 0;
    scoreMonitorListings[21].calculatationType = 1;
    scoreMonitorListings[21].scoreJumpForTrigger = 1;
    gameListings[21].ringSwitchCooldown = 2;

    writeStringToArray32("BAREKNUCKLE2", gameListings[22].gameId); // <-- Streets of Rage 2
    gameListings[22].ringByte = 0;
    gameListings[22].specialRingByte = 0;
    gameListings[22].livesBytes[0] = 0xEF82;
    gameListings[22].livesByteDestinations[0] = 0x5; 
    gameListings[22].timeBytes[0] = 0;
    gameListings[22].timeByteDestinations[0] = 1;
    gameListings[22].valueWriteDuration = 0;
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
    
    writeStringToArray32("BAREKNUCKLE3", gameListings[23].gameId); // <-- Streets of Rage 3
    gameListings[23].ringByte = 0;
    gameListings[23].specialRingByte = 0;
    gameListings[23].livesBytes[0] = 0xDF8A;
    gameListings[23].livesByteDestinations[0] = 0x5; 
    gameListings[23].timeBytes[0] = 0;
    gameListings[23].timeByteDestinations[0] = 1;
    gameListings[23].valueWriteDuration = 0;
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


    writeStringToArray32("THESUPERSHINOBI2", gameListings[24].gameId); // <-- Shinobi III
    gameListings[24].ringByte = 0;
    gameListings[24].specialRingByte = 0;
    gameListings[24].livesBytes[0] = 0x37E1;
    gameListings[24].livesBytes[1] = 0x37CD;
    gameListings[24].livesByteDestinations[0] = 0x5; 
    gameListings[24].livesByteDestinations[1] = 0x5; 
    gameListings[24].timeBytes[0] = 0;
    gameListings[24].timeByteDestinations[0] = 1;
    gameListings[24].valueWriteDuration = 0;
    scoreMonitorListings[24].scoreBytes[0] = 0x37B4;
    scoreMonitorListings[24].scoreBytes[1] = 0x37B5;
    scoreMonitorListings[24].scoreBytes[2] = 0x37B6;
    scoreMonitorListings[24].scoreBytesP2[0] = 0;
    scoreMonitorListings[24].scoreBytesP2[1] = 0;
    scoreMonitorListings[24].calculatationType = 1;
    scoreMonitorListings[24].scoreJumpForTrigger = 1;
    gameListings[24].ringSwitchCooldown = 2;

    writeStringToArray32("THESUPERSHINOBI", gameListings[25].gameId); // <-- Revenge of Shinobi
    gameListings[25].ringByte = 0;
    gameListings[25].specialRingByte = 0;
    gameListings[25].livesBytes[0] = 0xE140;
    gameListings[25].livesByteDestinations[0] = 0x5; 
    gameListings[25].timeBytes[0] = 0;
    gameListings[25].timeByteDestinations[0] = 1;
    gameListings[25].valueWriteDuration = 0;
    scoreMonitorListings[25].scoreBytes[0] = 0xFF12;
    scoreMonitorListings[25].scoreBytes[1] = 0xFF13;
    scoreMonitorListings[25].scoreBytesP2[0] = 0;
    scoreMonitorListings[25].scoreBytesP2[1] = 0;
    scoreMonitorListings[25].calculatationType = 1;
    scoreMonitorListings[25].scoreJumpForTrigger = 1;
    gameListings[25].ringSwitchCooldown = 2;

    writeStringToArray32("SHADOWDANCER", gameListings[26].gameId); // <-- SHADOWDANCER
    sprintf(gameAltIds[26], "534841444F574441 4E434552896582CC 9591000000000000 0000000000000000");
    gameListings[26].ringByte = 0;
    gameListings[26].specialRingByte = 0;
    gameListings[26].livesBytes[0] = 0x13DF;
    gameListings[26].livesByteDestinations[0] = 0x5; 
    gameListings[26].timeBytes[0] = 0;
    gameListings[26].timeByteDestinations[0] = 1;
    gameListings[26].valueWriteDuration = 0;
    scoreMonitorListings[26].scoreBytes[0] = 0x13E3;
    scoreMonitorListings[26].scoreBytes[1] = 0x13E0;
    scoreMonitorListings[26].scoreBytesP2[0] = 0;
    scoreMonitorListings[26].scoreBytesP2[1] = 0;
    scoreMonitorListings[26].calculatationType = 1;
    scoreMonitorListings[26].scoreJumpForTrigger = 0;
    gameListings[26].ringSwitchCooldown = 2;

    writeStringToArray32("MICROMACHINESII", gameListings[27].gameId); // <-- SHADOWDANCER
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


    writeStringToArray32("MicroMachines96", gameListings[28].gameId); // <-- Micro Machines 96 (has no header???)
    copyGameListing(27, 28);
    // the below comes from the fingerprint
    sprintf(gameAltIds[28], "393AFFFF9AE6C18A 052C4548784AA8CF 0667FCCA18000460 FCCA200045000000");
    gameListings[28].bytesToTestForChange[0] = 0xD164; // swap on new lap
    gameListings[28].bytesToTestForChange[1] = 0xCF7E; // swap on battle score change
    gameListings[28].livesBytes[0] = 0xE776;
    gameListings[28].livesByteDestinations[0] = 0x5; 

    writeStringToArray32("MicroMachines", gameListings[29].gameId); // <-- Micro Machines 96 (has no header???)
    sprintf(gameAltIds[29], "569A754E0061BAFF B94E000086A73C36 0000F94100001D9F 3C300E003C000000");
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

    writeStringToArray32("STREETSOFRAGE3", gameListings[30].gameId); // <-- Streets of Rage 3
    copyGameListing(23, 30);

    writeStringToArray32("STREETSOFRAGE2", gameListings[31].gameId); // <-- Streets of Rage 3
    copyGameListing(22, 31);

    writeStringToArray32("STREETSOFRAGE", gameListings[32].gameId); // <-- Streets of Rage 3
    copyGameListing(21, 32);

    writeStringToArray32("AfterBurnerII", gameListings[33].gameId);
    sprintf(gameAltIds[33], "B1CCC0B0CADEB0C5 B049490000000000 0000000000000000 0000000000000000");
    gameListings[33].ringByte = 0x06C8;

    writeStringToArray32("GUNSTARHEROES", gameListings[34].gameId);
    scoreMonitorListings[34].scoreBytes[0] = 0xA469;
    scoreMonitorListings[34].scoreBytes[1] = 0xA466;
    scoreMonitorListings[34].scoreBytes[2] = 0xA467;
    scoreMonitorListings[34].calculatationType = 1;
    scoreMonitorListings[34].scoreJumpForTrigger = 29;
    gameListings[34].ringSwitchCooldown = 2;


    // 08240 = Sonic 1 GG
    // 07250 = Sonic 2 GG
    // 15250 = Sonic Chaos GG
    // 30250 = triple trouble GG
    // 73250 = Blast GG

    // writeStringToArray32("CHAOTIX", gameListings[11].gameId); // Knuckles Chaotix 32x
    // writeStringToArray32("SONICCD", gameListings[11].gameId); // Sonic CD

    gameListingCount = 35;
    cartLoader_appendToLog("finished cartLoader_run");
}

void zeroAllListings() {
    for (int gameIndex = 0; gameIndex < MAX_ROMS; gameIndex++) {
        gameListings[gameIndex].ringByte = 0;
        gameListings[gameIndex].specialRingByte = 0;
        gameListings[gameIndex].valueWriteDuration = 0;
        gameListings[gameIndex].isISO = 0;
        gameListings[gameIndex].ringSwitchCooldown = 0;
        gameListings[gameIndex].unpauseByte = 0;

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

        for (int i = 0; i < 8; i++) {
            scoreMonitorListings[gameIndex].scoreBytes[i] = 0;
            scoreMonitorListings[gameIndex].scoreBytesP2[i] = 0;
        }
        scoreMonitorListings[gameIndex].calculatationType = 0 ;
        scoreMonitorListings[gameIndex].scoreJumpForTrigger = 0;
        scoreMonitorListings[gameIndex].blockJumpFromZero = 0;

        levelEditListings[gameIndex].startByte = 0;
        levelEditListings[gameIndex].endByte = 0;

        pixelMonitorListings[gameIndex].enabled = 0;

        for (int i = 0; i < 0x100; i++) {
            pixelStatesPerGame[gameIndex][i] = 0;
            pixelMonitorListings[gameIndex].xCoords[i] = 0;
            pixelMonitorListings[gameIndex].yCoords[i] = 0;
            pixelMonitorListings[gameIndex].allowedColours[i] = 0;

        }
    }
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

AAGameTransferListing cartLoader_getActiveGameTransferListing() {
    return gameTransferListings[cartLoader_getActiveCartIndex()];
}

AAScoreMonitorListing cartLoader_getActiveScoreMonitorListing() {
    return scoreMonitorListings[cartLoader_getActiveCartIndex()];
}

AALevelEditListing cartLoader_getActiveLevelEditListing() {
    return levelEditListings[cartLoader_getActiveCartIndex()];
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
    gameListings[toGame].isISO = gameListings[fromGame].isISO;
    gameListings[toGame].ringSwitchCooldown = gameListings[fromGame].ringSwitchCooldown;
    gameListings[toGame].accelerationType = gameListings[fromGame].accelerationType;
    gameListings[toGame].unpauseByte = gameListings[fromGame].unpauseByte;

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

    for (int i = 0; i < 8; i++) {
        scoreMonitorListings[toGame].scoreBytes[i] = scoreMonitorListings[fromGame].scoreBytes[i];
        scoreMonitorListings[toGame].scoreBytesP2[i] = scoreMonitorListings[fromGame].scoreBytesP2[i];
    }
    scoreMonitorListings[toGame].calculatationType = scoreMonitorListings[fromGame].calculatationType ;
    scoreMonitorListings[toGame].scoreJumpForTrigger = scoreMonitorListings[fromGame].scoreJumpForTrigger;
    scoreMonitorListings[toGame].blockJumpFromZero = scoreMonitorListings[fromGame].blockJumpFromZero;

    levelEditListings[toGame].startByte = levelEditListings[fromGame].startByte;
    levelEditListings[toGame].endByte = levelEditListings[fromGame].endByte;

    pixelMonitorListings[toGame].enabled = pixelMonitorListings[fromGame].enabled;
    for (int i = 0; i < 0x100; i++) {
        pixelMonitorListings[toGame].xCoords[i] = pixelMonitorListings[fromGame].xCoords[i];
        pixelMonitorListings[toGame].yCoords[i] = pixelMonitorListings[fromGame].yCoords[i];
        pixelMonitorListings[toGame].allowedColours[i] = pixelMonitorListings[fromGame].allowedColours[i];

    }
    pixelMonitorListings[toGame].changeMustAffectColour = pixelMonitorListings[fromGame].changeMustAffectColour;

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
            sprintf(path, "%s%s_%s.savestate", folderPath, romFilePrefixes[i], romFileNames[i]);
            
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
        sprintf(path, "%s%s_%s.savestate", folderPath, romFilePrefixes[i], romFileNames[i]);

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