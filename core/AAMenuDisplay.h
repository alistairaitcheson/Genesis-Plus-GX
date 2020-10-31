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

typedef struct {
    int infiniteLives;
    int infiniteTime;
    int copyVram;
    int switchGameType;
    int cooldownOnSwitch;
    int speedUpOnRing;
} HackOptions;

extern void menuDisplay_showMenu(int menuNum);
extern void menuDisplay_hideMenu();
extern int menuDisplay_onButtonPress(int buttonIndex);
extern HackOptions menuDisplay_getHackOptions();
extern void menuDisplay_initialise();
extern int menuDisplay_isShowing();

void beginGame();
void showTitleMenu();
void refreshMenu();
void showChooseGameMenu();
void incrementOption(int byAmount);
void showOptionsMenu();

void applySettingsFromArray256(int array256[]);
void applyDefaultSettings();
void saveHackOptions();

#endif