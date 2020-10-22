#include "AACartLoader.h"

#include "shared.h" // <--- this needs to be included at the top of every file, for compiler reasons I don't understand
#include <stdio.h>
#include <sys/types.h>
#include "include/dirent.h"
#include "vdp_render.h"

static unsigned int romCount;
static char *folderPath = "downloads/hackula/";
static char romFileNames[0x1000][0x100];

static char *logLines[0x100];
static unsigned int logLineCount;

void cartLoader_run() {
    listFiles(folderPath);
}


/**
 * Lists all files and sub-directories at given path.
 */
void listFiles(const char *path)
{
    struct dirent *dp;
    DIR *dir = opendir(path);
    remove("downloads/hackula/log_listFiles.txt");
    FILE *logWriter = fopen("downloads/hackula/log_listFiles.txt", "w");
    fprintf(logWriter, " --- ROM FILES --- \n");

    int yPos = 20;

    // Unable to open directory stream
    if (!dir) {
        fprintf(logWriter, "nothing found here...");
        fprintf(logWriter, "\n");
        fclose(logWriter);

        for (int i = 0; i < 200; i++) {
            vdp_setGraphicLayerPixel(0, i, yPos, 5);
        }
        return;
    } 

    while ((dp = readdir(dir)) != NULL)
    {
        for (int i = 0; i < 10; i++) {
            vdp_setGraphicLayerPixel(0, i, yPos, 5);
        }
        yPos += 10;

        if (pathIsRom(dp->d_name, dp->d_namlen) != 0) {
            char name[0x100];
            for (int i = 0; i < 0x100; i++) {
                if (i < dp->d_namlen) {
                    name[i] = dp->d_name[i];
                } else if (i == dp->d_namlen) {
                    name[i] = '\0';
                } else {
                    name[i] = 0;
                }
            }

            // because I can't figure out strings and pointers
            for (int i = 0; i < 0x100; i++) {
                romFileNames[romCount][i] = name[i]; 
            }
            romCount++;
            cartLoader_appendToLog("new-added rom");
            cartLoader_appendToLog(romFileNames[romCount]);

            // addRomListing(name);

            printf("%s\n", dp->d_name);
            fprintf(logWriter, dp->d_name);
            fprintf(logWriter, "\n");
        }
    }

    fclose(logWriter);

    // Close directory stream
    closedir(dir);
}

void cartLoader_loadRomAtIndex(int index) {
    cartLoader_appendToLog("building rom path to:");
    cartLoader_appendToLog(romFileNames[index]);

    char fullPath[0x100];
    int pathIndex = 0;
    for (int i = 0; i < 0x100; i++) {
        if (folderPath[i] == '\0') {
            break;
        } else {
            fullPath[i] = folderPath[i];
            pathIndex++;
        }
    }

    for (int i = 0; i < 0x100; i++) {
        if (romFileNames[index][i] == '\0') {
            break;
        } else {
            fullPath[pathIndex] = romFileNames[index][i];
            pathIndex++;
        }
    }
    fullPath[pathIndex] = '\0';

    cartLoader_appendToLog(fullPath);
    load_rom(fullPath);
    system_init();
    system_reset();
}

// void addRomListing(char *path) {
//     cartLoader_appendToLog("addRomListing");
//     cartLoader_appendToLog(path);
//     romFileNames[romCount] = path;
//     romCount++;

//     char str[0x100];
//     sprintf(str, "%d", romCount);
//     cartLoader_appendToLog(str);
// }

int pathIsRom(char *path, int pathLen) {
    if (pathLen > 4) {
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 's' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 'd' ) {
            return 1;
        }
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 'b' && path[pathLen - 2] == 'i' && path[pathLen - 1] == 'n' ) {
            return 1;
        }
        if (path[pathLen -3] == '.' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 'd') {
            return 1;
        }
    }
    return 0;
}

void cartLoader_appendToLog(char *text) {
    logLines[logLineCount % 0x100] = text;
    logLines[(logLineCount + 1) % 0x100] = "---";
    logLineCount++;

    remove("downloads/hackula/log.txt");
    FILE *logWriter = fopen("downloads/hackula/log.txt", "w");
    for (int i = 0; i < 0x100; i++) {
        fprintf(logWriter, logLines[i]);
        fprintf(logWriter, "\n");
    }
    fclose(logWriter);
}

void concatenate_string(char *original, char *add)
{
   while(*original)
      original++;
     
   while(*add)
   {
      *original = *add;
      add++;
      original++;
   }
   *original = '\0';
}

