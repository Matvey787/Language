#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "../../General/programTree/tree.h"
#include "workWithFile.h"
#include "refactorToTokens.h"
#include "createPredprocessingTree.h"
#include "../../General/constants.h"
#include "../../General/treeTransfer/treeTransfer.h"

int main(int argc, char* argv[])
{
    char* filePath = nullptr;
    if (argc == 2)
    {
        filePath = (char*)calloc(strlen(argv[1]) + 1, sizeof(char));
        snprintf(filePath, c_maxStrLen, "%s", argv[1]);
    }
    else
    {
        printf("Please, give a full path to .myl file.\n");
        return 1;
    }
    
    char* buffer = nullptr;
    size_t numOfSmbls = 0;
    size_t numOfStrs = 0;
    if (readFile(&buffer, filePath, &numOfSmbls, &numOfStrs) != NO_ERRORS)
    {
        return 1;
    }
    // array for saving system vars which are created during token processing
    char** systemVars = (char**)calloc(c_startSysVarsAmount, sizeof(char*));
    for (int i = 0; i < c_numberOfSysVars; i++)
    {
        systemVars[i] = (char*)calloc(c_lengthOfVarName, sizeof(char));
    }
    
    // table for saving user vars
    nameTable_t* nameTable = (nameTable_t*)calloc(c_nameTableSize, sizeof(nameTable_t));

    node_t* tokens = createTokens(buffer, numOfSmbls, nameTable, systemVars, "../dot_files/tokensDotFile.dot", "../png_files");
    node_t* predprocessingTree = createPredprocessingTree(tokens, "../dot_files/frontenedDotFile.dot", "../png_files");
    
    pushTree(predprocessingTree, "../progTree");
    free(filePath);
    free(nameTable);
    delTree(predprocessingTree);
    free(tokens);
    free(buffer);

    for (int i = 0; i < c_numberOfSysVars; i++)
        free(systemVars[i]);
    free(systemVars);

    systemVars = nullptr; // FIXME
    predprocessingTree = nullptr;
    tokens = nullptr;
    nameTable = nullptr;
    buffer = nullptr;
    return 0;
}
