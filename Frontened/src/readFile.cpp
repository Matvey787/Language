#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "workWithFile.h"

error readFile(char** buffer, const char* file_name, size_t* numOfSymbols, size_t* numOfStrs){
    // check if the pointers are not null
    assert(buffer       != nullptr);
    assert(file_name    != nullptr);
    assert(numOfStrs    != nullptr);
    assert(numOfSymbols != nullptr);

    // open file for read
    FILE* rFile = fopen(file_name, "rb");

    // if the file can't be opened, return an error
    if (rFile == nullptr)
    {
        printf("can't open read file\n");
        return OPEN_FILE_FAIL;
    }

    // find size of file
    fseek(rFile, 0, SEEK_END);
    *numOfSymbols = (size_t)ftell(rFile);
    fseek(rFile, 0, SEEK_SET);

    // read text from file
    *buffer = (char*)calloc(*numOfSymbols, sizeof(char));

    // if the memory can't be allocated, return an error
    if (*buffer == nullptr)
    {
        printf("allocate memory fail");
        return ALLOCATE_MEMORY_FAIL;
    }

    // read the file into the buffer
    fread(*buffer, sizeof(char), *numOfSymbols, rFile);

    // add '\n' at the end of the buffer if it doesn't exist
    if ((*buffer)[*numOfSymbols - 1] != '\n')
    {
        *buffer = (char*)realloc(*buffer, sizeof(char) * (*numOfSymbols + 1));
        (*buffer)[*numOfSymbols] = '\n';
        ++(*numOfSymbols);
    }

    // find number of strings
    for (size_t i = 0; i < *numOfSymbols; i++) if ((*buffer)[i] == '\n') ++(*numOfStrs);

    fclose(rFile);
    return NO_ERRORS;
}