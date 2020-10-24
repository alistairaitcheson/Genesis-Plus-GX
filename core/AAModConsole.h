#ifndef _AAMODCONSOLE_H_
#define _AAMODCONSOLE_H_

#include "AACommonTypes.h"

extern void modConsole_initialise();

extern void modConsole_updateFrame();

extern void modConsole_getRomHeader(char intoArray[]);
extern int modconsole_array32sAreEqual(char arrayA[], char arrayB[]);

void updateSpeedUpOnRing();
int ringCountHasChanged();

#endif