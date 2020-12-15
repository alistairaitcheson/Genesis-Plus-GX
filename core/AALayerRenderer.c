#include "AALayerRenderer.h"

#include "shared.h" // <--- this needs to be included at the top of every file, for compiler reasons I don't understand
#include "AACartLoader.h"

static LetterListing letters[0x100];

void layerRenderer_populateLetters() {
    populateLetter('A',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b01000010,
                    0b01111110,
                    0b01000010,
                    0b01000010,
                    0b00000000);

    populateLetter('B',
                    0b00000000,
                    0b01111100,
                    0b01000010,
                    0b01111100,
                    0b01000010,
                    0b01000010,
                    0b01111100,
                    0b00000000);

    populateLetter('C',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b01000000,
                    0b01000000,
                    0b01000010,
                    0b00111100,
                    0b00000000);

    populateLetter('D',
                    0b00000000,
                    0b01111100,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01111100,
                    0b00000000);
    populateLetter('E',
                    0b00000000,
                    0b01111110,
                    0b01000000,
                    0b01111100,
                    0b01000000,
                    0b01000000,
                    0b01111110,
                    0b00000000);
    populateLetter('F',
                    0b00000000,
                    0b01111110,
                    0b01000000,
                    0b01111100,
                    0b01000000,
                    0b01000000,
                    0b01000000,
                    0b00000000);
    populateLetter('G',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b01000000,
                    0b01000110,
                    0b01000010,
                    0b00111100,
                    0b00000000);
    populateLetter('H',
                    0b00000000,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01111110,
                    0b01000010,
                    0b01000010,
                    0b00000000);
    populateLetter('I',
                    0b00000000,
                    0b00111000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00111000,
                    0b00000000);
    populateLetter('J',
                    0b00000000,
                    0b01111110,
                    0b00000100,
                    0b00000100,
                    0b00000100,
                    0b01000100,
                    0b00111000,
                    0b00000000);
    populateLetter('K',
                    0b00000000,
                    0b01000100,
                    0b01001000,
                    0b01010000,
                    0b01101000,
                    0b01000100,
                    0b01000010,
                    0b00000000);
    populateLetter('L',
                    0b00000000,
                    0b01000000,
                    0b01000000,
                    0b01000000,
                    0b01000000,
                    0b01000000,
                    0b01111110,
                    0b00000000);
    populateLetter('M',
                    0b00000000,
                    0b01111110,
                    0b01010010,
                    0b01010010,
                    0b01010010,
                    0b01000010,
                    0b01000010,
                    0b00000000);
    populateLetter('N',
                    0b00000000,
                    0b01000010,
                    0b01100010,
                    0b01010010,
                    0b01001010,
                    0b01000110,
                    0b01000010,
                    0b00000000);
    populateLetter('O',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b00111100,
                    0b00000000);
    populateLetter('P',
                    0b00000000,
                    0b01111100,
                    0b01000010,
                    0b01000010,
                    0b01111100,
                    0b01000000,
                    0b01000000,
                    0b00000000);
    populateLetter('Q',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01001010,
                    0b00111100,
                    0b00001000);
    populateLetter('R',
                    0b00000000,
                    0b01111100,
                    0b01000010,
                    0b01000010,
                    0b01111100,
                    0b01000100,
                    0b01000010,
                    0b00000000);
    populateLetter('S',
                    0b00000000,
                    0b00111110,
                    0b01000000,
                    0b01000000,
                    0b00111100,
                    0b00000010,
                    0b01111100,
                    0b00000000);
    populateLetter('T',
                    0b00000000,
                    0b01111100,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00000000);
    populateLetter('U',
                    0b00000000,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b00111100,
                    0b00000000);
    populateLetter('V',
                    0b00000000,
                    0b01000100,
                    0b01000100,
                    0b01000100,
                    0b01000100,
                    0b00101000,
                    0b00010000,
                    0b00000000);
    populateLetter('W',
                    0b00000000,
                    0b01000010,
                    0b01000010,
                    0b01000010,
                    0b01010010,
                    0b01010010,
                    0b01111110,
                    0b00000000);
    populateLetter('X',
                    0b00000000,
                    0b01000100,
                    0b00101000,
                    0b00010000,
                    0b00101000,
                    0b01000100,
                    0b01000100,
                    0b00000000);
    populateLetter('Y',
                    0b00000000,
                    0b01000100,
                    0b01000100,
                    0b00101000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00000000);
    populateLetter('Z',
                    0b00000000,
                    0b01111110,
                    0b00000100,
                    0b00001000,
                    0b00010000,
                    0b00100000,
                    0b01111110,
                    0b00000000);

    duplicateLetter('A', 'a');
    duplicateLetter('B', 'b');
    duplicateLetter('C', 'c');
    duplicateLetter('D', 'd');
    duplicateLetter('E', 'e');
    duplicateLetter('F', 'f');
    duplicateLetter('G', 'g');
    duplicateLetter('H', 'h');
    duplicateLetter('I', 'i');
    duplicateLetter('J', 'j');
    duplicateLetter('K', 'k');
    duplicateLetter('L', 'l');
    duplicateLetter('M', 'm');
    duplicateLetter('N', 'n');
    duplicateLetter('O', 'o');
    duplicateLetter('P', 'p');
    duplicateLetter('Q', 'q');
    duplicateLetter('R', 'r');
    duplicateLetter('S', 's');
    duplicateLetter('T', 't');
    duplicateLetter('U', 'u');
    duplicateLetter('V', 'v');
    duplicateLetter('W', 'w');
    duplicateLetter('X', 'x');
    duplicateLetter('Y', 'y');
    duplicateLetter('Z', 'z');

    populateLetter('0',
                    0b00000000,
                    0b00111100,
                    0b01000110,
                    0b01001010,
                    0b01010010,
                    0b01100010,
                    0b00111100,
                    0b00000000);
    populateLetter('1',
                    0b00000000,
                    0b00001000,
                    0b00011000,
                    0b00001000,
                    0b00001000,
                    0b00001000,
                    0b00011100,
                    0b00000000);
    populateLetter('2',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b00000100,
                    0b00011000,
                    0b0010000,
                    0b01111110,
                    0b00000000);
    populateLetter('3',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b00001100,
                    0b00000010,
                    0b01000010,
                    0b00111100,
                    0b00000000);
    populateLetter('4',
                    0b00000000,
                    0b00001100,
                    0b00010100,
                    0b00100100,
                    0b01000100,
                    0b01111110,
                    0b00000100,
                    0b00000000);
    populateLetter('5',
                    0b00000000,
                    0b01111110,
                    0b01000000,
                    0b01111100,
                    0b00000010,
                    0b01000010,
                    0b00111100,
                    0b00000000);
    populateLetter('6',
                    0b00000000,
                    0b00111100,
                    0b01000000,
                    0b01111100,
                    0b01000010,
                    0b01000010,
                    0b00111100,
                    0b00000000);
    populateLetter('7',
                    0b00000000,
                    0b01111110,
                    0b00000100,
                    0b00001000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00000000);
    populateLetter('8',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b00111100,
                    0b01000010,
                    0b01000010,
                    0b00111100,
                    0b00000000);
    populateLetter('9',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b00111110,
                    0b00000010,
                    0b00000010,
                    0b00111100,
                    0b00000000);
    populateLetter('.',
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b01100000,
                    0b01100000,
                    0b00000000);
    populateLetter('-',
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00111100,
                    0b00000000,
                    0b00000000,
                    0b00000000);
    populateLetter('&',
                    0b00000000,
                    0b00110000,
                    0b01001000,
                    0b00110000,
                    0b01001010,
                    0b01000100,
                    0b00111010,
                    0b00000000);
    populateLetter('\'',
                    0b00000000,
                    0b00110000,
                    0b00010000,
                    0b00100000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000);
    populateLetter('+',
                    0b00000000,
                    0b00000000,
                    0b00010000,
                    0b00010000,
                    0b01111100,
                    0b00010000,
                    0b00010000,
                    0b00000000);
    populateLetter('(',
                    0b00000000,
                    0b00000100,
                    0b00001000,
                    0b00001000,
                    0b00001000,
                    0b00001000,
                    0b00000100,
                    0b00000000);
    populateLetter(')',
                    0b00000000,
                    0b00100000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00100000,
                    0b00000000);

    populateLetter('<',
                    0b00000000,
                    0b00000000,
                    0b00010000,
                    0b00100000,
                    0b01000000,
                    0b00100000,
                    0b00010000,
                    0b00000000);

    populateLetter('>',
                    0b00000000,
                    0b00000000,
                    0b00001000,
                    0b00000100,
                    0b00000010,
                    0b00000100,
                    0b00001000,
                    0b00000000);

    populateLetter('%',
                    0b00000000,
                    0b01100010,
                    0b01100100,
                    0b00001000,
                    0b00010000,
                    0b00100110,
                    0b01000110,
                    0b00000000);
    populateLetter(':',
                    0b00000000,
                    0b01100000,
                    0b01100000,
                    0b00000000,
                    0b00000000,
                    0b01100000,
                    0b01100000,
                    0b00000000);
    populateLetter('?',
                    0b00000000,
                    0b00111100,
                    0b01000010,
                    0b00000010,
                    0b00011100,
                    0b00000000,
                    0b00011000,
                    0b00000000);
    populateLetter('*',
                    0b00000000,
                    0b00010000,
                    0b01111100,
                    0b00101000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000);
    populateLetter('/',
                    0b00000000,
                    0b00000010,
                    0b00000100,
                    0b00001000,
                    0b00010000,
                    0b00100000,
                    0b01000000,
                    0b00000000);
    populateLetter('_',
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b00000000,
                    0b01111110,
                    0b00000000);
    populateLetter('!',
                    0b00000000,
                    0b00011000,
                    0b00011000,
                    0b00011000,
                    0b00011000,
                    0b00000000,
                    0b00011000,
                    0b00000000);
    populateLetter('[',
                    0b00000000,
                    0b00001110,
                    0b00001000,
                    0b00001000,
                    0b00001000,
                    0b00001000,
                    0b00001110,
                    0b00000000);
    populateLetter(']',
                    0b00000000,
                    0b01110000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b00010000,
                    0b01110000,
                    0b00000000);
}

void populateLetter(char identifier, unsigned char line0, unsigned char line1, unsigned char line2, unsigned char line3, unsigned char line4, unsigned char line5, unsigned char line6, unsigned char line7) {
    letters[identifier].lines[0] = line0;
    letters[identifier].lines[1] = line1;
    letters[identifier].lines[2] = line2;
    letters[identifier].lines[3] = line3;
    letters[identifier].lines[4] = line4;
    letters[identifier].lines[5] = line5;
    letters[identifier].lines[6] = line6;
    letters[identifier].lines[7] = line7;

    letters[identifier].populated = 1;
}

void duplicateLetter(char fromLetter, char toLetter) {
    for (int  i = 0; i < 8; i++) {
        letters[toLetter].lines[i] = letters[fromLetter].lines[i];
    }
    letters[toLetter].populated = 1;
}

void layerRenderer_clearLayer(unsigned int layer) {
    vdp_clearGraphicLayer(layer);
}

void layerRenderer_fill(unsigned int layer, unsigned int startX, unsigned int startY, unsigned int width, unsigned int height, unsigned int value) {
    for (int x = startX; x < startX + width; x++) {
        for (int y = startY; y < startY + height; y++) {
            vdp_setGraphicLayerPixel(layer, x, y, value);
        }
    }
}

void layerRenderer_writeLetter(unsigned int layer, unsigned int startX, unsigned int startY, char letter, unsigned int value) {
    // cartLoader_appendToLog("layerRenderer_writeLetter: character");
    // char charLog[0x10];
    // charLog[0] = letter;
    // cartLoader_appendToLog(charLog);
    // for (int i = 0; i < 8; i++) {
    //     sprintf(charLog, "%d", letters[letter].lines[i]);
    //     cartLoader_appendToLog(charLog);
    // }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (getPixelFromLetter(letter, x, y) != 0) {
                vdp_setGraphicLayerPixel(layer, startX + x, startY + y, value);
            }
        }
    }
}

void layerRenderer_writeWord256(unsigned int layer, unsigned int startX, unsigned int startY, char word256[], unsigned int value) {
    // cartLoader_appendToLog("layerRenderer_writeWord256");
    // cartLoader_appendToLog(word256);

    unsigned int x = 0;
    unsigned int y = 0;
    for (int i = 0; i < 0x100; i++) {
        char letter = word256[i];
        if (letter == '\0') {
            break;
        } else if (letter == '\n') {
            x = 0;
            y += 8;
        } else {
            layerRenderer_writeLetter(layer, startX + x, startY + y, letter, value);
            x += 8;
        }
    }
}

void layerRenderer_writeWord256WithBorder(unsigned int layer, unsigned int startX, unsigned int startY, char word256[], unsigned int value, unsigned int borderThickness, unsigned int borderValue) {
    for (int oX = -borderThickness; oX <= borderThickness; oX ++) {
        for (int oY = -borderThickness; oY <= borderThickness; oY ++) {
            layerRenderer_writeWord256(layer, startX + oX, startY + oY, word256, borderValue);
        }
    }
    layerRenderer_writeWord256(layer, startX, startY, word256, value);
}

void layerRenderer_writeWord256Centred(unsigned int layer, unsigned int startX, unsigned int startY, char word256[], unsigned int value) {
    int length = 0;
    for (int i = 0; i < 256; i++) {
        if (word256[i] == 0 || word256[i] == '\0') {
            break;
        }
        length++;
    }
    layerRenderer_writeWord256(layer, startX - (length * 4), startY - 4, word256, value);
}

unsigned int getPixelFromLetter(char letter, unsigned int x, unsigned int y) {
    // if (x < 8 && y < 8) {
        unsigned int row = (unsigned int)letters[letter].lines[y];

        int index = 7 - x;

        unsigned int testNum = 1;
        for (int i = 0; i < index; i++) {
            testNum = testNum * 2;
        }

        unsigned int result = row & testNum;
        if ((int)result == 0) {
            return 0;
        } else {
            return 1;
        }
    // }
    // return 0;
}