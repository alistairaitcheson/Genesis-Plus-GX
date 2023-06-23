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
#define NETWORK_MSG_HEAL_COLOURS 'L'

#define NETWORK_MSG_DUMMY_TWTICH_MESSAGE 'P'
#define NETWORK_MSG_REQUEST_RAM_STATE 'p'
#define NETWORK_MSG_SEND_GAME_INDEX_START '['
#define NETWORK_MSG_SEND_GAME_INDEX_END ']'
#define NETWORK_MSG_GAME_IS_MS '#'
#define NETWORK_MSG_GAME_IS_GENESIS '$'
#define NETWORK_MSG_IS_FROM_TWITCH '@'
#define NETWORK_MSG_IS_SET_VRAM_STATE 'a'

#define NETWORK_MSG_WRITE_TO_RAM 'q'
#define NETWORK_MSG_TOGGLE_LAYER 'w'
#define NETWORK_MSG_WRITE_TO_CART 'e'
#define NETWORK_MSG_WRITE_SPECIFIC_TO_RAM 'r'
#define NETWORK_MSG_USE_ACTIVE_NUM_AS_LOCATION 't'
#define NETWORK_MSG_USE_ACTIVE_NUM_AS_DISTANCE 'y'
#define NETWORK_MSG_USE_ACTIVE_NUM_AS_HOLD_DURATION 'u'

#define NETWORK_MSG_INTERPRET_AS_ACTIONS 'A' // prefix with this to convey "I am actioning"
#define NETWORK_MSG_INTERPRET_AS_RULES 'S' // prefix with this to convey "I am switching rules on and off"
#define NETWORK_MSG_INTERPRET_AS_POSITIVE 'D' // prefix with this to convey "Set the next setting you see to ON"
#define NETWORK_MSG_INTERPRET_AS_NEGATIVE 'F' // prefix with this to convey "Set the next setting you see to OFF"

#define NETWORK_MSG_REQUEST_RULES 'Z' // when called, this emu will reply with the current rules setup so the opponent can sync 
#define NETWORK_MSG_REQUEST_OPPONENT_SEED 'z' // when called, this emu will reply with the seed for 999 challenge
#define NETWORK_MSG_APPLY_OPPONENT_SEED 'x' // when called, this emu will reply with the seed for 999 challenge
#define NETWORK_MSG_APPLY_OPPONENT_RING_COUNT 'v' // when called, this emu will reply with the seed for 999 challenge
#define NETWORK_MSG_APPLY_OPPONENT_HAS_COMPLETED_CHALLENGE 's'
#define NETWORK_MSG_RECEIVE_RINGS_FROM_OPPONENT 'd'
#define NETWORK_MSG_ENFORCE_SONIC_SPEED 'X'
// to do: implement this!!
#define NETWORK_MSG_SHOW_TERMINAL_MENU 'C' // when using a USB terminal, send this to say "show the hack select menu please"
#define NETWORK_MSG_ACTIVATE_RULE_PRESET 'V' // precede this with a number, e.g. 0V = no rules, 1V = "make it switch game on get ring"
#define NETWORK_MSG_FIRE_TERMINAL_ACTION 'B' // precede this with a number: 1 = kill Sonic, 2 = reset game, 3 = rewind 1 step
#define NETWORK_MSG_START_SPECIFIC_GAME 'N' // precede this with a number - swap to the game with that index (0N = switch to 0th game)
#define NETWORK_MSG_ISOLATE_SPECIFIC_GAME 'M' // precede this with a number - swap to the game with that index (0N = switch to 0th game) and make it the only active game

#define NETWORK_MSG_ADD_TO_ROTOR_POSITION 'm' // precede it with a number to add to that rotor
#define NETWORK_MSG_REMOVE_FROM_ROTOR_POSITION 'n' // precede it with a number to remove from that rotor
#define NETWORK_MSG_REPORT_TERMINAL_EVENT '?' // follow it with a number: ?0 = get ring, ?1 = swap game
#define NETWORK_MSG_HAS_LED_DISPLAY 'b' // will be sent when an LED display is detected

#define NETWORK_RECEIVE_OPPONENT_LEVEL_COMPLETION_COUNT 'c'


#define NETWORK_INTERPRET_TYPE_ACTION 0
#define NETWORK_INTERPRET_TYPE_ASSIGN_RULES 1


#include "AACommonTypes.h"

extern void modConsole_initialise();

extern void modConsole_updateFrame();

extern void modConsole_getRomHeader(char intoArray[]);
extern void modConsole_getLockOnRomHeader(char intoArray[]);
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
extern void modConsole_processNetworkEvent(char eventId, int eventCount, int eventLocation, int eventDistance, int isFromTwitch, int holdDuration);
void queueNetworkMessage(char eventId);
void sendQueuedNetworkMessage();
extern void applyLayerHidingOptions();
extern int getBigRandomNumber(int maxValue);

int lengthOfString256(char string256[]);

void applyHeldValues();
void checkDeathCounter();

void updateSpeedUpOnRing();
int ringCountHasChanged(int shouldIgnoreCooldown);
int standingHasChanged(int shouldIgnoreCooldown);
void updateLives();
void updateTime() ;
void updateSwitchGameOnRing();
void updateSwitchGameOnLand();
void showRomList();
extern void promptSwitchGame();
void switchGame();
void clearCooldownVisualiser();
void showCooldownVisualiser();
void overwriteLevelOnRing();
void overwriteLevel(int cycleCount, int overwriteType);
void sendNetworkMessageOnGetRing();
void sendNetworkMessageOnHitBoss();

void unpauseGame();
extern void fireSnapEffect(int isFromTwitch);
void shuffleSnapValues(int isFromTwitch);
extern int modConsole_getSnapOffsetForRowIndex(int rowIndex);
void applyRandomiseVelocity();
void updateRandomiseVelocityOnRing();

void healColoursByTime();
void healColoursOnRing(int count);
void removeColourOnRing(int count);

void initialiseRewindRAM();
void cacheRewindRAM();
extern int stepBackRewindRAM();

void showRewindSymbol();
void hideRewindSymbol();
void fireScreenSnapOnEvent();
void applyRamEditOnRing();
int checkForBossDefeats();

void writeWRAMintoSpriteBuffer();
void writeWRAMintoLevelLayout();

void forceSonicSpeed(unsigned int amount);

extern void modConsole_beginRewindAction();
extern void modConsole_endRewindAction();
void applyVramEditOnRing();

int pendingRingTriggerShouldFire();
void updatePendingRingTrigger();
void increasePendingRingTriggers(int count);
int getBossRushElapsedFrames();
int getBossRushElapsedSecs();
int getBossRushElapsedMins();
int getBossRushElapsedHours();
void resetBossRushElapsedTimer();
void beginCountdownToApplyBossRushRings();
void dismissStartupHint(int andSave);
void zeroDeathCount();
extern void setShouldShuffleController(int toValue);
extern void setShouldCheckForIdleMode(int toValue);
void incrementTerminalRotorValue(int whichRotor, int amount);
void resetRotorRam();
void resetRotorChanges();
void setHasLEDDisplay(int toValue);
void reportToLED(char actionId[]) ;
void endIdleMode();
int getIsIdleModeActive();

void checkToHaltMusic();
void enforceHaltMusic();

int getDeathCount();
void checkForBossHits(int asNetwork, int challengeIndex);

int getNinesChallengeElapsedFrames();
void resetNinesChallengeElapsedTimer();
int getNinesChallengeElapsedSecs();
int getNinesChallengeElapsedMins();
int getNinesChallengeElapsedHours();
void requestFlashRingsToGo();

void bumpEventCountForSwitchGame();
int deductFromRingCount(int amount);
void resetNinesOpponentLeadCountdown();
int receiveRingsFromOpponent(int ringCount);

#endif