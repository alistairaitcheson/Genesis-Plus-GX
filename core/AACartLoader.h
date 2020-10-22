#ifndef _AA_CART_LOADER_H_
#define _AA_CART_LOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern void cartLoader_run();
void listFiles(const char *path);
void addRomListing(char *path);
int pathIsRom(char *path, int pathLen);

#endif