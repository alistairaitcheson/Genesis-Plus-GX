#include "AACartLoader.h"

#include "shared.h" // <--- this needs to be included at the top of every file, for compiler reasons I don't understand
#include <stdio.h>
#include <sys/types.h>
#include "include/dirent.h"
#include "vdp_render.h"

static unsigned int romCount;
static char *folderPath = "downloads/hackula/";
static char *romFileNames[0x1000];

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
            addRomListing(dp->d_name);

            printf("%s\n", dp->d_name);
            fprintf(logWriter, dp->d_name);
            fprintf(logWriter, "\n");
        }
    }

    fclose(logWriter);

    // Close directory stream
    closedir(dir);
}

void addRomListing(char *path) {
    
}

int pathIsRom(char *path, int pathLen) {
    if (pathLen > 4) {
        if (path[pathLen -4] == '.' && path[pathLen - 3] == 's' && path[pathLen - 2] == 'm' && path[pathLen - 1] == 'd' ) {
            return 1;
        }
    }
    return 0;
}