/*

Open Terminal

cd C:\Users\agaitcheson\Documents\Development\Emulation\GenesisPlusGIT\Genesis-Plus-GX
cls
"C:\Program Files (x86)\GnuWin32\bin\make.exe" -f Makefile.libretro

*/

#include "shared.h" // <--- this needs to be included at the top of every file, for compiler reasons I don't understand
#include "AAModConsole.h"
#include "AACommonTypes.h"
#include "genesis.h"
#include "AACartLoader.h"
#include "gamepad.h"
#include "input.h"
#include "AALayerRenderer.h"

static AAModType activeModType = AAMODTYPE_SPEED_UP_ON_RING;

static unsigned int frameCount = 0;

static AAGameListing activeGameListing;

static uint16 lastPadState;
static uint16 padState;

void modConsole_initialise() {
    layerRenderer_populateLetters();
    cartLoader_run();
}

void modConsole_updateActiveCart() {
    char romHeader[0x20];
    modConsole_getRomHeader(romHeader);
    activeGameListing = cartLoader_getActiveGameListing();

    cartLoader_appendToLog("modConsole_updateActiveCart - You are playing:");
    cartLoader_appendToLog(activeGameListing.gameId);

    layerRenderer_writeWord256(0, 0, 0, "Hello World!", 5);
    layerRenderer_writeWord256(0, 0, 0, "!@#$&*()", 5);
    layerRenderer_writeWord256(0, 0, 100, activeGameListing.gameId, 6);

}

void modConsole_updateFrame() {
    lastPadState = padState;
    padState = input.pad[0];

    // if (padState != lastPadState) {
    //     char padStateAsString[0x20];
    //     sprintf(padStateAsString, "%d", padState);
    //     cartLoader_appendToLog(padStateAsString);
    // }

    if (activeModType == AAMODTYPE_SPEED_UP_ON_RING) {
        updateSpeedUpOnRing();
    }

    updateLives();
    updateTime();

    if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
        buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
        buttonStateAtIndex(INPUT_INDEX_A) != 0) {
        modConsole_activatePanic();
    }
    if (buttonStateAtIndex(INPUT_INDEX_UP) != 0 &&
        buttonStateAtIndex(INPUT_INDEX_START) != 0 &&
        buttonStateAtIndex(INPUT_INDEX_A) != 0 &&
        buttonStateAtIndex(INPUT_INDEX_B) != 0) {
        modConsole_activateReset();
    }

    // vdp_clearGraphicLayer(0);
    // layerRenderer_writeWord256(0, 0, 0, "Hello World!", 5);
    // layerRenderer_writeWord256(0, 0, 100, activeGameListing.gameId, 6);

    // vdp_clearGraphicLayer(0);

    // for (int i = 0; i < 0x10; i++) {
    //     int value = buttonStateAtIndex(i);
    //     if(value != 0) {
    //         for (int j = 0; j < 20; j++) {
    //             vdp_setGraphicLayerPixel(0, (i + 1) * 5, j + value, 5);
    //         }
    //     }
    // }

    // int x = 0;
    // int y = 20;
    // for (int i = 0; i < padState; i++) {
    //     vdp_setGraphicLayerPixel(0, x, y, 6);
    //     x++;
    //     if (x >= 0x100) {
    //         x = 0;
    //         y++;
    //     }
    // }

    frameCount++;

    aa_genesis_updateLastRam();
}

void updateLives() {
    for (int i = 0; i < 3; i++) {
        if (activeGameListing.livesBytes[i] != 0) {
            aa_genesis_setWorkRam(activeGameListing.livesBytes[i], activeGameListing.livesByteDestinations[i]);
        } else {
            break;
        }
    }
}

void updateTime() {
    for (int i = 0; i < 3; i++) {
        if (activeGameListing.timeBytes[i] != 0) {
            aa_genesis_setWorkRam(activeGameListing.timeBytes[i], activeGameListing.timeByteDestinations[i]);
        } else {
            break;
        }
    }
}

void modConsole_activatePanic() {
    for (int i = 0; i < 3; i++) {
        if (activeGameListing.panicBytes[i] != 0) {
            aa_genesis_setWorkRam(activeGameListing.panicBytes[i], activeGameListing.panicByteDestinations[i]);
        } else {
            break;
        }
    }
}

void modConsole_activateReset() {
    system_reset();
    aa_genesis_updateLastRam();
}

void updateSpeedUpOnRing() {
    if (ringCountHasChanged() != 0) {
        // speed
        aa_genesis_incrementWorkRamCompoundValueByInt(0xF760, 2, 0x40);
        // acceleration
        aa_genesis_incrementWorkRamCompoundValueByInt(0xF762, 2, 0x08);
    }
}

int ringCountHasChanged() {
    if (activeGameListing.ringByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(activeGameListing.ringByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(activeGameListing.ringByte);

        if (currentRingCount != 0 && currentRingCount != lastRingCount) {
            return 1;
        }
    }

    if (activeGameListing.specialRingByte > 0) {
        unsigned int lastRingCount = aa_genesis_getLastWorkRam(activeGameListing.specialRingByte);
        unsigned int currentRingCount = aa_genesis_getWorkRam(activeGameListing.specialRingByte);
        int difference = abs((int)lastRingCount - (int)currentRingCount);

        if (currentRingCount != 0 && currentRingCount != lastRingCount && difference < 6) {
            return 1;
        }
    }

    return 0;
}

void modConsole_getRomHeader(char intoArray[]) {
    if (cart.romsize == 0x400000) {
        writeStringToArray32("SONIC3&KNUCKLES", intoArray);
        return;
    }

    uint8 tempHeader[0x20];
    for (int i = 0; i < 0x20; i++) {
        tempHeader[i] = 0;
    }

    uint8 byteArray[0x20];
    for (int i = 0; i < 0x20; i++) {
        int index = 0x100 + 0x20 + i;
        if (i % 2 == 0) {
            index += 1;
        } else {
            index -= 1;
        }
        uint8 character = getCartValueAtIndex(index);
        tempHeader[i] = character;
    }
    
    uint8 tidiedHeader[0x20];
    int tempIndex = 0;
    for (int i = 0; i < 0x20; i++) {
        if (tempHeader[i] != 0 && tempHeader[i] != ' ') {
            tidiedHeader[tempIndex] = tempHeader[i];
            tempIndex++;
        }
    }
    if (tempIndex < 0x20) {
        tidiedHeader[tempIndex] = '\0';
    }

    for (int i = 0; i < 0x20; i++) {
        if (i <= tempIndex) {
            intoArray[i] = tidiedHeader[i];
        }
        else {
            intoArray[i] = 0;
        }
    }
}

int modconsole_array32sAreEqual(char arrayA[], char arrayB[]) {
    // cartLoader_appendToLog("modconsole_array32sAreEqual");
    // cartLoader_appendToLog(arrayA);
    // cartLoader_appendToLog(arrayB);

    for (int i = 0; i < 0x20; i++) {
        if (arrayA[i] != arrayB[i]) {
            return 0;
        }
        if (arrayA[i] == '\0') {
            break;
        }
    }
    return 1;
}

int getButtonState(uint16 whichInput) {
    // char padStateAsString[0x20];
    // sprintf(padStateAsString, "%d", padState & whichInput);
    // cartLoader_appendToLog(padStateAsString);

    if (padState & whichInput != 0) {
        return 1;
    }
    return 0;
}

int getLastButtonState(uint16 whichInput) {
    if (lastPadState & whichInput != 0) {
        return 1;
    }
    return 0;
}

int buttonWasReleased(uint16 whichInput) {
    if (getLastButtonState(whichInput) != 0 && getButtonState(whichInput) == 0) {
        return 1;
    }
    return 0;
}

int buttonWasPressed(uint16 whichInput) {
    if (getLastButtonState(whichInput) == 0 && getButtonState(whichInput) != 0) {
        return 1;
    }
    return 0;
}

int buttonWasReleasedAtIndex(int index) {
    if (lastButtonStateAtIndex(index) != 0 && buttonStateAtIndex(index) == 0) {
        return 1;
    }
    return 0;
}

int buttonWasPressedAtIndex(int index) {
    if (lastButtonStateAtIndex(index) == 0 && buttonStateAtIndex(index) != 0) {
        return 1;
    }
    return 0;
}

int lastButtonStateAtIndex(int index) {
    uint testNum = 1;
    for (int i = 0; i < index; i++) {
        testNum = testNum * 2;
    }

    int result = ((int)((padState % 0x100) & testNum) % 0x100);

    for (int i = 0; i < 0x10; i++) {
        for (int j = 0; j < testNum; j++) {
            vdp_setGraphicLayerPixel(0, j, (index + 3) * 10, 9);
        }
        vdp_setGraphicLayerPixel(0, result, ((index + 3) * 10) + 1, 5);
    }

    if (result == 0) { // why does this always return false?
        return 0;
    } else {
        return 1;
    }
}

int buttonStateAtIndex(int index) {
    uint testNum = 1;
    for (int i = 0; i < index; i++) {
        testNum = testNum * 2;
    }

    int result = ((int)((padState % 0x100) & testNum) % 0x100);

    for (int i = 0; i < 0x10; i++) {
        for (int j = 0; j < testNum; j++) {
            vdp_setGraphicLayerPixel(0, j, (index + 3) * 10, 9);
        }
        vdp_setGraphicLayerPixel(0, result, ((index + 3) * 10) + 1, 5);
    }

    if (result == 0) {
        return 0;
    } else {
        return 1;
    }
}