#ifndef _AAMODCONSOLE_H_
#define _AAMODCONSOLE_H_

#define NETWORK_MSG_SWITCH_GAME 'Q'
#define NETWORK_MSG_SPEED_UP 'W'
#define NETWORK_MSG_SCRAMBLE_LEVEL_EASY 'E'
#define NETWORK_MSG_SCRAMBLE_LEVEL_MEDIUM 'R'
#define NETWORK_MSG_SCRAMBLE_LEVEL_HARD 'T'
#define NETWORK_MSG_RANDOMISE_VELOCITY 'Y'
#define NETWORK_MSG_REMOVE_COLOUR 'U'
#define NETWORK_MSG_REMOVE_10_COLOURS 'I'

#define NETWORK_MSG_DUMMY_TWTICH_MESSAGE 'P'
#define NETWORK_MSG_SEND_GAME_INDEX_START '['
#define NETWORK_MSG_SEND_GAME_INDEX_END ']'
#define NETWORK_MSG_IS_FROM_TWITCH '@'
#define NETWORK_MSG_IS_SET_VRAM_STATE 'a'

#define NETWORK_MSG_WRITE_TO_RAM 'q'
#define NETWORK_MSG_TOGGLE_LAYER 'w'
#define NETWORK_MSG_WRITE_TO_CART 'e'

#define NETWORK_MSG_INTERPRET_AS_ACTIONS 'A' // prefix with this to convey "I am actioning"
#define NETWORK_MSG_INTERPRET_AS_RULES 'S' // prefix with this to convey "I am switching rules on and off"
#define NETWORK_MSG_INTERPRET_AS_POSITIVE 'D' // prefix with this to convey "Set the next setting you see to ON"
#define NETWORK_MSG_INTERPRET_AS_NEGATIVE 'F' // prefix with this to convey "Set the next setting you see to OFF"

#define NETWORK_MSG_REQUEST_RULES 'Z'

#define NETWORK_INTERPRET_TYPE_ACTION 0
#define NETWORK_INTERPRET_TYPE_ASSIGN_RULES 1


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
extern void modConsole_processNetworkEvent(char eventId, int eventCount, int isFromTwitch);
void queueNetworkMessage(char eventId);
void sendQueuedNetworkMessage();
extern void applyLayerHidingOptions();
extern int getBigRandomNumber(int maxValue);

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
void sendNetworkMessageOnGetRing();

void unpauseGame();
void fireSnapEffect(int isFromTwitch);
void shuffleSnapValues(int isFromTwitch);
extern int modConsole_getSnapOffsetForRowIndex(int rowIndex);
void applyRandomiseVelocity();
void updateRandomiseVelocityOnRing();

void healColoursByTime();
void healColoursOnRing(int count);
void removeColourOnRing(int count);

void initialiseRewindRAM();
void cacheRewindRAM();
extern void stepBackRewindRAM();

#endif