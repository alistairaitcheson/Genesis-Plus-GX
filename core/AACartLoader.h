#ifndef _AA_CART_LOADER_H_
#define _AA_CART_LOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "AACommonTypes.h"

extern void cartLoader_run();
extern void cartLoader_appendToLog(char *text);
extern int cartLoader_getActiveCartIndex();
extern AAGameListing cartLoader_getActiveGameListing();

void listFiles(const char *path);
void addRomListing(char *path);
int pathIsRom(char *path, int pathLen);
void cartLoader_loadRomAtIndex(int index);
void concatenate_string(char *original, char *add);
void writeStringToArray32(char *source, char dest[]);
void copyGameListing(AAGameListing fromGame, AAGameListing toGame);

#endif