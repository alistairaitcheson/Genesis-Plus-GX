#ifndef _AA_LAYER_RENDERER_H_
#define _AA_LAYER_RENDERER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "AACommonTypes.h"

extern void layerRenderer_clearLayer(unsigned int layer);
extern void layerRenderer_writeLetter(unsigned int layer, unsigned int startX, unsigned int startY, char letter, unsigned int value);
extern void layerRenderer_fill(unsigned int layer, unsigned int startX, unsigned int startY, unsigned int width, unsigned int height, unsigned int value);
extern void layerRenderer_writeWord256(unsigned int layer, unsigned int startX, unsigned int startY, char word256[], unsigned int value);
extern void layerRenderer_populateLetters();
extern void layerRenderer_writeWord256Centred(unsigned int layer, unsigned int startX, unsigned int startY, char word256[], unsigned int value);
extern void layerRenderer_writeWord256WithBorder(unsigned int layer, unsigned int startX, unsigned int startY, char word256[], unsigned int value, unsigned int borderThickness, unsigned int borderValue);

void populateLetter(char identifier, unsigned char line0, unsigned char line1, unsigned char line2, unsigned char line3, unsigned char line4, unsigned char line5, unsigned char line6, unsigned char line7);
void duplicateLetter(char fromLetter, char toLetter);
unsigned int getPixelFromLetter(char letter, unsigned int x, unsigned int y);

#endif