/*

Open Terminal

cd C:\Users\agaitcheson\Documents\Development\Emulation\GenesisPlusGIT\Genesis-Plus-GX
"C:\Program Files (x86)\GnuWin32\bin\make.exe" -f Makefile.libretro

*/

#include "AAModConsole.h"
#include "AACommonTypes.h"
#include "genesis.h"

static AAModType activeModType = AAMODTYPE_SPEED_UP_ON_RING;

static unsigned int ringCountByte;
static unsigned int specialRingCountByte;

void modConsole_updateFrame() {
    if (activeModType == AAMODTYPE_SPEED_UP_ON_RING) {
        updateSpeedUpOnRing();
    }

    aa_genesis_upateLastRam();
}

void updateSpeedUpOnRing() {
    if (ringCountHasChanged() != 0) {
        
    }
}

int ringCountHasChanged() {
    if (ringCountByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(ringCountByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(ringCountByte);

        if (currentRingCount != 0 && currentRingCount != lastRingCount) {
            return 1;
        }
    }

    if (specialRingCountByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(specialRingCountByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(specialRingCountByte);
        int difference = abs((int)lastRingCount - (int)currentRingCount);

        if (currentRingCount != 0 && currentRingCount != lastRingCount && difference < 6) {
            return 1;
        }
    }

    return 0;
}