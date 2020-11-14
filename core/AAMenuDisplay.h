#ifndef _AA_MENU_DISPLAY_H_
#define _AA_MENU_DISPLAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "types.h"

#define MENU_LISTING_NONE 0
#define MENU_LISTING_TITLE 1
#define MENU_LISTING_CHOOSE_GAME 2
#define MENU_LISTING_SETTINGS 3
#define MENU_LISTING_IN_GAME 4
#define MENU_LISTING_RANDOMISED_ROMS 5
#define MENU_LISTING_PERSIST_VALUES 6

typedef struct {
    int infiniteLives;
    int infiniteTime;
    int copyVram;
    int switchGameType;
    int cooldownOnSwitch;
    int speedUpOnRing;
    int loadFromSavedState;
    int automaticallySaveStatesFreq;
    int shouldWriteToLog;
} HackOptions;

typedef struct {
    int lives;
    int rings;
    int topSpeed;
    int momentum;
    int time;
} PersistValuesOptions;

extern void menuDisplay_showMenu(int menuNum);
extern void menuDisplay_hideMenu();
extern int menuDisplay_onButtonPress(int buttonIndex);
extern HackOptions menuDisplay_getHackOptions();
extern void menuDisplay_initialise();
extern int menuDisplay_isShowing();
extern void menuDisplay_hideMenuUnlessQueued();
extern PersistValuesOptions menuDisplay_getPersistValuesOptions();

void beginGame();
void showTitleMenu();
void refreshMenu();
void showChooseGameMenu();
void incrementOption(int byAmount);
void showOptionsMenu();
void showRandomisedGameMenu();
void showPersistValuesMenu();
void togglePersistValue(int index);

void applyPersistValuesFromArray256(int array256[]);
void applyDefaultPersistValues();

void applySettingsFromArray256(int array256[]);
void applyDefaultSettings();
void saveHackOptions();

void activateInGameMenuItem();
void showInGameOptionsMenu();

#endif