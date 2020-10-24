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
    int livesBytes[3];
    int livesByteDestinations[3];
    int timeBytes[3];
    int timeByteDesitnations[3];
    int panicBytes[2];
    int panicByteDesitnations[2];
} AAGameListing;


#endif