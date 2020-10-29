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
#define MENU_LISTING_SETTINGS 2

extern void menuDisplay_showMenu(int menuNum);
extern void menuDisplay_hideMenu();
extern int menuDisplay_onButtonPress(int buttonIndex);

void beginGame();
void showTitleMenu();
void refreshMenu();
void showChooseGameMenu();

#endif