#include "AAMenuDisplay.h"

#include "shared.h"
#include <stdio.h>
#include <sys/types.h>
#include "include/dirent.h"
#include "vdp_render.h"
#include "AAModConsole.h"
#include "AACommonTypes.h"
#include <sys/stat.h>
#include "AALayerRenderer.h"


static int activeMenu = MENU_LISTING_NONE;

static int chosenGameIndex = 0;
static int optionsItemIndex = 0;

static int majorVersion = 0;
static int minorVersion = 3;

void menuDisplay_showMenu(int menuNum) {
    activeMenu = menuNum;
}

void menuDisplay_hideMenu() {

}

void beginGame() {

}

void menuDisplay_onButtonPress(uint16 buttonId) {

}

void showStartMenu() {

}