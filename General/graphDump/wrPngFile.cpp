#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graphDump.h"
#include "../constants.h"

int writePngFile(const char* dotFile, const char* directory, const char* backGrColor)
{    
    assert(directory != nullptr);
    assert(dotFile != nullptr);
    static int numOfCall = 0;

    char command[300] = {0};

    // get only name of file path
    char fileName[tempStrLength] = {0};
    size_t k = 0;
    for (size_t i = 0; i < strlen(dotFile); i++)
    {
        if (dotFile[i] == '/')
        {
            k = 0;
            i++;
        }
        fileName[k++] = dotFile[i];
    }
    fileName[k] = '\0';

    sprintf(command, "dot %s -Tpng -Gbgcolor=%s -o %s/%s.png", dotFile, backGrColor, directory, fileName);
    system(command);

    return numOfCall;
}