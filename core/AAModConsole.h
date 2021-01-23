#ifndef _AAMODCONSOLE_H_
#define _AAMODCONSOLE_H_

#define NETWORK_MSG_SWITCH_GAME 'Q'
#define NETWORK_MSG_SPEED_UP 'W'
#define NETWORK_MSG_SCRAMBLE_LEVEL_EASY 'E'
#define NETWORK_MSG_SCRAMBLE_LEVEL_MEDIUM 'R'
#define NETWORK_MSG_SCRAMBLE_LEVEL_HARD 'T'
#define NETWORK_MSG_RANDOMISE_VELOCITY 'Y'

#include "AACommonTypes.h"

extern void modConsole_initialise();

extern void modConsole_updateFrame();

extern void modConsole_getRomHeader(char intoArray[]);
extern void modConsole_getMasterSystemProductId(char intoArray[]);
extern int modconsole_array32sAreEqual(char arrayA[], char arrayB[]);
extern void modConsole_updateActiveCart();
void modConsole_getRomFingerprint(char intoArray[], int location);

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
extern void modConsole_queuePanic();

extern void modConsole_applyHackOptions();
extern void modConsole_applyNetworkOptions();
extern void modConsole_flagToApplyCache();
extern void modConsole_flagToSummonMenu();
extern void modConsole_flagToLogRamState();
extern void modConsole_setCountdownUntilRingSwitch(int toValue);
extern void modConsole_processNetworkEvent(char eventId);
void queueNetworkMessage(char eventId);
void sendQueuedNetworkMessage();

int lengthOfString256(char string256[]);

void updateSpeedUpOnRing();
int ringCountHasChanged();
void updateLives();
void updateTime() ;
void updateSwitchGameOnRing();
void showRomList();
extern void promptSwitchGame();
void switchGame();
void clearCooldownVisualiser();
void showCooldownVisualiser();
void overwriteLevelOnRing();
void overwriteLevel(int cycleCount, int overwriteType);

void unpauseGame();
void fireSnapEffect();
void shuffleSnapValues();
extern int modConsole_getSnapOffsetForRowIndex(int rowIndex);

#endif