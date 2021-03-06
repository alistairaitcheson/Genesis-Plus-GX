#ifndef _AA_COMMON_TYPES_H_
#define _AA_COMMON_TYPES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum {
    AAMODTYPE_NONE,
    AAMODTYPE_SPEED_UP_ON_RING,
    AAMODTYPE_SWITCH_GAME,
}AAModType;

typedef struct
{
    char gameId[0x20];
    int ringByte;
    int specialRingByte;
    int updateHUDFlags[8];
    int livesBytes[8];
    int livesByteDestinations[8];
    int timeBytes[8];
    int timeByteDestinations[8];
    int panicBytes[8];
    int panicByteDestinations[8];
    int accelerationType;
    int valueWriteDuration;
    int isISO;
    int ringSwitchCooldown;
} AAGameListing;

typedef struct
{
    int ringBytesForTransfer[8];
    int speedBytesForTransfer[8];
    int timeBytesForTransfer[8];
    int momentumBytesForTransfer[8];
    int scoreBytesForTransfer[8];
} AAGameTransferListing;

typedef struct
{
    int scoreBytes[8];
    int scoreBytesP2[8];
    int delayBetweenChanges;
    int calculatationType; // 0 = add the values of the bytes (like Sonic 2 rings), 1 = 0x12 should be read as 12 (like Sonic 3D rings), 2 = each byte is a single place value (like Mean Bean Machine)
    int scoreJumpForTrigger;
    int scoreSwitchCooldown;
    int blockJumpFromZero;
    // int significantScoreThresholds[8]; // e.g. switch when your score goes up by [A, B, C, ...] - or when it goes past this threshold? Which is more interesting? Probably the latter
} AAScoreMonitorListing;

typedef struct {
    unsigned char lines[8];
    int populated;
} LetterListing;

static int INPUT_INDEX_UP = 0;
static int INPUT_INDEX_DOWN = 1;
static int INPUT_INDEX_LEFT = 2;
static int INPUT_INDEX_RIGHT = 3;
static int INPUT_INDEX_A = 4;
static int INPUT_INDEX_B = 5;
static int INPUT_INDEX_C = 6;
static int INPUT_INDEX_START = 7;

#endif