/*

Open Terminal

cd C:\Users\agaitcheson\Documents\Development\Emulation\GenesisPlusGIT\Genesis-Plus-GX
"C:\Program Files (x86)\GnuWin32\bin\make.exe" -f Makefile.libretro

*/

#include "AAModConsole.h"
#include "AACommonTypes.h"

static AAModType activeModType = AAMODTYPE_SPEED_UP_ON_RING;

void modConsole_updateFrame() {
    if (activeModType == AAMODTYPE_SPEED_UP_ON_RING) {
        updateSpeedUpOnRing();
    }
}

void updateSpeedUpOnRing() {

}