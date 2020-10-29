#ifndef _AA_MENU_DISPLAY_H_
#define _AA_MENU_DISPLAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "types.h"

static int MENU_LISTING_NONE = 0;
static int MENU_LISTING_TITLE = 1;
static int MENU_LISTING_CHOOSE_GAME = 2;
static int MENU_LISTING_SETTINGS = 2;

extern void menuDisplay_showMenu(int menuNum);
extern void menuDisplay_hideMenu();
extern void menuDisplay_onButtonPress(uint16 buttonId);

void beginGame();

#endif