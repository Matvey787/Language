#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

// from General
#include "../../General/constants.h"
#include "errors.h"
#include "programTree/tree.h"
#include "graphDump/graphDump.h"

#include "refactorToTokens.h"


// Special local function for correct write tokens to .dot file
static error wrTokensToDot(const node_t* tokens, const size_t numOfTokens, nameTable_t* nameTable, const size_t lengthOfNameTable, const char* tokensDotFile);

// Local function for checking if buffer is not empty.
// Return 1 if we can continue our parsing or return 0 if the end of buff
static bool bufferIsEmpty(char* buffer, char* last_addr);

// Local function for getting variable name from buffer by info in nameTable_t 
static const char* getVariableFromTable(nameTable_t* nameTable, size_t index);

#define processToken_(type, symbolStrs)                            \
    else if (strncmp(buffer, symbolStrs, strlen(symbolStrs)) == 0) \
    {                                                              \
        tokens[i_toks++] = {ND_##type, {0}, nullptr, nullptr};     \
        buffer += strlen(symbolStrs);                              \
    }


//TODO nameTable + tokens
node_t* createTokens(char* buffer, const size_t l_buff, nameTable_t* nameTable, char** systemVars, const char* tokensDotFile, const char* directoryForSavingPictures)
{
    // need for scanning end of buffer
    char* last_addr = buffer + l_buff;

    node_t* tokens = (node_t*)calloc(c_numberOfTokens, sizeof(node_t));
    size_t i_toks = 0;
    size_t i_sysVars = 0;
    size_t i_nameTab = 0;

    while (!bufferIsEmpty(buffer, last_addr))
    {
        
        // it is needed for scaning interval in for
        if (strncmp(buffer, "..", 2) == 0)
        {
            strcpy(systemVars[i_sysVars], "_end");
            nameTable[i_nameTab].str          = systemVars[i_sysVars];
            nameTable[i_nameTab].numOfSymbols = strlen("_end");
            tokens[i_toks]                    = {ND_ENDFOR, {0}, nullptr, nullptr};
            tokens[i_toks].data.var           = &nameTable[i_nameTab];
            
            buffer += 2;
            ++i_sysVars;
            ++i_toks;
            ++i_nameTab;
        }

        else if (*buffer == '\n')
        {
            tokens[i_toks++] = {ND_SEP, {0}, nullptr, nullptr};
            buffer += 1;
        }

        // code generation. This is where the macros are deployed
        #include "codeGeneration_(refactorToTokens).h"

        else if (isalpha(*buffer))
        {
            nameTable[i_nameTab].str = buffer;
            while (isalpha(*buffer)) 
            {
                nameTable[i_nameTab].numOfSymbols += 1;
                ++buffer;
            }
            tokens[i_toks] = {ND_VAR, {0}, nullptr, nullptr};
            tokens[i_toks].data.var = &nameTable[i_nameTab];
            ++i_toks;
            ++i_nameTab;
        }

        else if (isdigit(*buffer))
        {
            double tmpNum = 0;
            const size_t TMP_STR_SIZE = 100;
            char tmpStr[TMP_STR_SIZE] = {0};
            sscanf(buffer, "%lg", &tmpNum);
            
            tokens[i_toks] = {ND_NUM, {0}, nullptr, nullptr};
            tokens[i_toks].data.num = tmpNum;
            
            snprintf(tmpStr, TMP_STR_SIZE, "%lg", tmpNum);
            buffer += strlen(tmpStr);
            ++i_toks;
        }
        else 
        {
            buffer += 1;
        }
    }
    //                    \/ End of translation
    tokens[i_toks++] = {ND_EOT, {0}, nullptr, nullptr};
    wrTokensToDot(tokens, i_toks, nameTable, i_nameTab, tokensDotFile);
    writePngFile(tokensDotFile, directoryForSavingPictures, "black");

    return tokens;
}

#undef processTocen_

static bool bufferIsEmpty(char* buffer, char* last_addr)
{
    assert(buffer != nullptr);

    if (buffer == last_addr)
        return true;
    else
        return false;
}

static error wrTokensToDot(const node_t* tokens, const size_t numOfTokens, nameTable_t* nameTable,
                           const size_t lengthOfNameTable, const char* tokensDotFile)
{
    assert(tokens        != nullptr);
    assert(nameTable     != nullptr);
    assert(tokensDotFile != nullptr);

    FILE* wFile = fopen(tokensDotFile, "w");
    if (wFile == nullptr){
        printf("couldn't open file");
        return OPEN_FILE_FAIL;
    }

    fprintf(wFile, "digraph G\n{ \nrankdir=TB;\n newrank=true;\n");
    fprintf(wFile, "\n");

    fprintf(wFile, "{\n\trank=same; \n");

    for (size_t i = 0; i < numOfTokens; i++)
    {
        if (tokens[i].type == ND_NUM)
            fprintf(wFile, "\ttoken_%lu [ shape=record, color = %s, fontcolor = %s, label = \"{ (%s) | %lg }\" ];\n",
            i, getColor(tokens[i].type), getColor(tokens[i].type), convertTypeToStr(tokens[i].type), tokens[i].data.num);
        else
            fprintf(wFile, "\ttoken_%lu [ shape=record, color = %s, fontcolor = %s, label = \"{ (%s) }\" ];\n",
            i, getColor(tokens[i].type), getColor(tokens[i].type), convertTypeToStr(tokens[i].type));
    }
    fprintf(wFile, "}\n");
    
    fprintf(wFile, "{\n\trank=same; \n");
    for (size_t i = 0; i < lengthOfNameTable; i++)
    {
        fprintf(wFile, "\ttableCell_%p [ shape=record, color = %s, fontcolor = %s, label = \" \
        { buffer addr: %p | length: %lu }\" ];\n",
        nameTable[i].str, "green", "green", nameTable[i].str, nameTable[i].numOfSymbols);
    }
    fprintf(wFile, "}\n");


    fprintf(wFile, "{\n\trank=same; \n");
    for (size_t i = 0; i < lengthOfNameTable; i++)
    {
        fprintf(wFile, "\topenTableCell_%p [ shape=record, color = %s, fontcolor = %s, label = \"{ %s }\" ];\n",
        nameTable[i].str, "green", "green", getVariableFromTable(nameTable, i));
    }
    fprintf(wFile, "}\n");

    for (size_t i = 0, j = 0; i < numOfTokens; i++)
    {
        if (tokens[i].type == ND_VAR || tokens[i].type == ND_ENDFOR)
        {
            fprintf(wFile, "\ttoken_%lu:s -> tableCell_%p:n [ color = green; ]\n", i, nameTable[j++].str);
        }
    }

    for (size_t i = 0; i < lengthOfNameTable; i++)
    {
        fprintf(wFile, "\ttableCell_%p:s -> openTableCell_%p:n [ color = green; ]\n", nameTable[i].str, nameTable[i].str);
    }

    fprintf(wFile, "}\n");
    fclose(wFile);

    return NO_ERRORS;
}

static const char* getVariableFromTable(nameTable_t* nameTable, size_t index)
{
    *(nameTable[index].str + nameTable[index].numOfSymbols) = '\0';
    return nameTable[index].str;
}
