#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "programTree/tree.h"
#include "errors.h"

#include "convertToASM.h"

const size_t c_numOfVars = 100;

typedef struct {
    node_t* node;
    node_t* func;
    node_t* funcVars[c_numOfVars];
    size_t numOfFuncVars;
    short side;
} recInfo_t;

static void recConvert(recInfo_t* info, FILE** wFile);
static void handleVarsInFunc(recInfo_t* info);
static void handleEquation(recInfo_t* info, FILE** wFile);
static void pasteBaseInfo(const char* basicStartFile, FILE** wFile);
static size_t findFuncVar(recInfo_t* info, node_t* varNode);
static size_t addFuncVar   (recInfo_t* info, node_t* varNode);

#define GO_DEEPER_(func)        \
    if (leftNode != nullptr)    \
    {                           \
        info->node = leftNode;  \
        func(info, wFile);      \
        info->node = currNode;  \
    }                           \
                                \
    if (rightNode != nullptr)   \
    {                           \
        info->node = rightNode; \
        func(info, wFile);      \
        info->node = currNode;  \
    }

void writeNASM64(node_t* node, const char* asmFile)
{
    recInfo_t info = {0};
    info.node = node;
    FILE* wFile = fopen(asmFile, "wb");

    if (wFile == nullptr)
    {
        printf("Error opening file\n");
        return;
    }
    recConvert(&info, &wFile);

    fclose(wFile);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

static void recConvert(recInfo_t* info, FILE** wFile)
{
    node_t* currNode = info->node;
    if (currNode == nullptr) return;
    node_t* leftNode = info->node->left;
    node_t* rightNode = info->node->right;
    switch (currNode->type)
    {
    case ND_PR:
    {
        node_t* arg = currNode->left;
        if (arg->type == ND_VAR)
        {
            fprintf(*wFile, "mov edi, [rbp - %lu]\n\tcall printNum\n\t", findFuncVar(info, arg) * 4);
        }
        else if (arg->type == ND_NUM)
        {
            fprintf(*wFile, "mov edi, %d\n\tcall printNum\n\t", (int)arg->data.num);
        }
        return;      
    }
    case ND_EQ:
    {
        node_t* var = currNode->left;
        node_t* val = currNode->right;

        info->node = val;
        handleEquation(info, wFile);
        info->node = currNode;

        fprintf(*wFile, "pop rax\n\tmov [rbp - %lu], eax\n\t", findFuncVar(info, var)  * 4);
        break;
    }
    case ND_FUNCALL:
    {
        fprintf(*wFile, "call %s\n\t", currNode->data.var->str);

        break;
    }
    case ND_FUN:
    {
        info->func = currNode;
        memset(info->funcVars, 0, sizeof(info->funcVars));

        //check main
        if (strcmp("main", currNode->data.var->str) == 0)
        {
            pasteBaseInfo("../base.ASM", wFile);
            fprintf(*wFile, "_start:\n\tpush rbp\n\tmov rbp, rsp\n\t");
        }
        else
        {
            fprintf(*wFile, "%s:\n\tpush rbp\n\tmov rbp, rsp\n\t", currNode->data.var->str);
        }
        

        info->node = rightNode;
        handleVarsInFunc(info);
        info->node = currNode;
        fprintf(*wFile, "sub rsp, %lu\n\t", (info->numOfFuncVars / 16 + 1) * 16);

        info->node = rightNode;
        recConvert(info, wFile);
        info->node = currNode;

        if (strcmp("main", currNode->data.var->str) == 0)
        {
            fprintf(*wFile, "FINISH\n\t");
        }
        else
        {
            fprintf(*wFile, "mov rsp, rbp\n\tpop rbp\n\tret\n\t");
        }
        
        break;
    }
    case ND_SEP:
    {
        GO_DEEPER_(recConvert)

        break;
    }
    default:
    {
        break;
    }
    }
    
}

static void pasteBaseInfo(const char* basicStartFile, FILE** wFile)
{
    assert(basicStartFile != nullptr);
    assert(wFile != nullptr);
    assert(*wFile != nullptr);

    FILE* inputFile = fopen(basicStartFile, "rb");
    if (inputFile == nullptr)
    {
        perror("Error opening basicStartFile");
        return;
    }

    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    if (fileSize < 0)
    {
        perror("Error determining file size");
        fclose(inputFile);
        return;
    }

    char* buffer = (char*)calloc((size_t)fileSize, sizeof(char));
    if (buffer == nullptr)
    {
        perror("Error allocating buffer");
        fclose(inputFile);
        return;
    }

    size_t bytesRead = fread(buffer, 1, (size_t)fileSize, inputFile);
    if (bytesRead != (size_t)fileSize)
    {
        perror("Error reading basicStartFile");
        free(buffer);
        fclose(inputFile);
        return;
    }

    size_t bytesWritten = fwrite(buffer, 1, (size_t)fileSize, *wFile);
    if (bytesWritten != (size_t)fileSize)
    {
        perror("Error writing to wFile");
        free(buffer);
        fclose(inputFile);
        return;
    }

    free(buffer);
    fclose(inputFile);
}

static void handleVarsInFunc(recInfo_t* info)
{
    node_t* currNode = info->node;
    types currNodeType = currNode->type;
    node_t* leftNode = info->node->left;
    node_t* rightNode = info->node->right;

    if (currNodeType == ND_VAR)
    {
        findFuncVar(info, currNode);
    }

    if (leftNode != nullptr)
    {
        info->node = leftNode;
        handleVarsInFunc(info);    
    }

    if (rightNode != nullptr)
    {
        info->node = rightNode;
        handleVarsInFunc(info); 
    }
}

static void handleEquation(recInfo_t* info, FILE** wFile)
{
    node_t* currNode  = info->node;
    node_t* leftNode  = info->node->left;
    node_t* rightNode = info->node->right;

    types currNodeType = info->node->type;

    // size_t freeSpaceStackFrame = (info->numOfFuncVars + 1) * 4;

    switch (currNodeType)
    {
    case ND_ADD:
    {
        GO_DEEPER_(handleEquation)

        fprintf(*wFile, "pop rax\n\tpop rbx\n\tadd rax, rbx\n\tpush rax\n\t");

        break;
    }
    case ND_SUB:
    {
        GO_DEEPER_(handleEquation)

        fprintf(*wFile, "pop rbx\n\tpop rax\n\tsub rax, rbx\n\tpush rax\n\t");

        break;
    }
    case ND_MUL:
    {
        GO_DEEPER_(handleEquation)

        fprintf(*wFile, "pop rbx\n\tpop rax\n\tmul rbx\n\tpush rax\n\\t");

        break;
    }
    case ND_DIV:
    {
        GO_DEEPER_(handleEquation)

        fprintf(*wFile, "pop rbx\n\tpop rax\n\tdiv rbx\n\tpush rax\n\t");

        break;
    }
    case ND_VAR:
    {
        
        fprintf(*wFile, "mov rax, [rbp - %lu]\n\tpush rax\n\t", findFuncVar(info, currNode) * 4);

        break;
    }
    case ND_NUM:
    {
        fprintf(*wFile, "push %d\n\t", (int)currNode->data.num);

        break;
    }
    
    default:
    {
        break;
    }
    }
}

#pragma GCC diagnostic pop

static size_t addFuncVar(recInfo_t* info, node_t* varNode)
{
    assert(info->funcVars != nullptr);

    for (size_t i = 0; i < c_numOfVars; ++i)
    {
        if (info->funcVars[i] == nullptr)
        {
            info->funcVars[i] = varNode;
            info->numOfFuncVars += 1;
            return info->numOfFuncVars;
        }
    }
}

static size_t findFuncVar(recInfo_t* info, node_t* varNode)
{
    assert(info->funcVars != nullptr);
    assert(varNode != nullptr);

    char* nameOfCurrVar = varNode->data.var->str;

    for (size_t varIndex = 0; varIndex < info->numOfFuncVars; ++varIndex)
    {
        char* funcVar = info->funcVars[varIndex]->data.var->str;

        if (strcmp(funcVar, nameOfCurrVar) == 0) return varIndex + 1;
    }
    return addFuncVar(info, varNode);
}
