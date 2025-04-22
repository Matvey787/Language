#ifndef WORKWITHFILE
#define WORKWITHFILE

#include <stdio.h>
#include "../../General/errors.h"

/**
 * @brief This function reads the contents of a file into a string.
 * 
 * @param buffer A pointer to a pointer to a character array where the file contents will be stored.
 * @param file_name The path to the file to be read.
 * @param numOfSymbols The number of symbols in the file. (just only where to put the information)
 * @param numOfStrs The number of strings in the file. (just only where to put the information)
 * 
 * @return A string containing the contents of the file, or an empty string if the file could not be read.
 */
error readFile(char** buffer, const char* file_name, size_t* numOfSymbols, size_t* numOfStrs);

#endif // WORKWITHFILE