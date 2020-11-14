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
    int ringBytesForTransfer[8];
    int livesBytes[8];
    int livesByteDestinations[8];
    int timeBytes[8];
    int timeByteDestinations[8];
    int panicBytes[8];
    int panicByteDestinations[8];
    int speedBytesForTransfer[8];
    int timeBytesForTransfer[8];
    int momentumBytesForTransfer[8];
} AAGameListing;

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