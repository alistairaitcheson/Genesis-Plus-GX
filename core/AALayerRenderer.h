#ifndef _AA_LAYER_RENDERER_H_
#define _AA_LAYER_RENDERER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern void layerRenderer_clearLayer(unsigned int layer, unsigned int x, unsigned int y, unsigned int value);
extern void layerRenderer_writeLetter(unsigned int layer, unsigned int x, unsigned int y, char letter);

#endif