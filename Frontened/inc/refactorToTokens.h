#ifndef REFACTORTOTOKENS
#define REFACTORTOTOKENS

#include <stdio.h>
#include "../../General/programTree/tree.h"

/// @brief This function splits the string into tokens. The tokens are stored in the array of nodes.
/// @param buffer the string to split on tokens
/// @param l_buff the length of the string
/// @param nameTable the table for saving variables (faster then copy substrings)
/// @param systemVars the array of system variables
/// @param tokensDotFile // the file for writing tokens and make graphic dump 
/// @param directoryForSavingPictures // the directory for saving pictures
/// @return The pointer to tokens array

node_t* createTokens(char* buffer, const size_t l_buff, nameTable_t* nameTable, char** systemVars, const char* tokensDotFile, const char* directoryForSavingPictures);

#endif // REFACTORTOTOKENS