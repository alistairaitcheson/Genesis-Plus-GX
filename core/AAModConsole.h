#ifndef _AAMODCONSOLE_H_
#define _AAMODCONSOLE_H_

#include "AACommonTypes.h"

extern void modConsole_initialise();

extern void modConsole_updateFrame();

extern void modConsole_getRomHeader(char intoArray[]);
extern int modconsole_array32sAreEqual(char arrayA[], char arrayB[]);
extern void modConsole_updateActiveCart();

extern int getButtonState(uint16 whichInput);
extern int getLastButtonState(uint16 whichInput);
extern int buttonWasReleased(uint16 whichInput);
extern int buttonWasPressed(uint16 whichInput);
extern int lastButtonStateAtIndex(int index);
extern int buttonStateAtIndex(int index);
extern int buttonWasReleasedAtIndex(int index);
extern int buttonWasPressedAtIndex(int index);

extern void modConsole_activatePanic();
extern void modConsole_activateReset();


void updateSpeedUpOnRing();
int ringCountHasChanged();
void updateLives();
void updateTime() ;
void updateSwitchGameOnRing();
void showRomList();

#endif