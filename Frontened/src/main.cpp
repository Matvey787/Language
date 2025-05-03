#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "../../General/programTree/tree.h"
#include "workWithFile.h"
// #include "createPredprocessingTree.h"
#include "../../General/constants.h"
// #include "../../General/treeTransfer/treeTransfer.h"

#include "tokenizer.h"
#include "predprocessor.h"

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

    size_t numOfTokens = 0;

    node_t* tokens = tokenize(buffer, numOfSmbls, &numOfTokens);
    drawTokens(tokens, numOfTokens, "../png_files", "tokens");

    node_t* predprocessingTree = createTree(tokens);



    freeTokens(tokens, numOfTokens);
    free(filePath);
    delTree(predprocessingTree);
    free(buffer);

        // node_t* predprocessingTree = createPredprocessingTree(tokens, "../dot_files/frontenedDotFile.dot", "../png_files");
    
    // pushTree(predprocessingTree, "../progTree");
    
    // free(nameTable);
    // free(tokens);

    // for (int i = 0; i < c_numberOfSysVars; i++)
    //     free(systemVars[i]);
    // free(systemVars);

    // systemVars = nullptr; // FIXME
    // predprocessingTree = nullptr;
    // tokens = nullptr;
    // nameTable = nullptr;
    // buffer = nullptr;
    // return 0;
}
