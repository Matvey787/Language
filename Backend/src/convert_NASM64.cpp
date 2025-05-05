#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "programTree/tree.h"
#include "errors.h"

#include "convertToASM.h"

const size_t c_numOfVars = 100;

const size_t numOfCallerRegs = 6;

enum argsStep_t
{
    e_edi = 1,
    e_esi = 2,
    e_edx = 3,
    e_ecx = 4,
    e_r8d = 5,
    e_r9d = 6,
    e_stack = 7
};

struct funcInfo_t
{
    node_t* func;
    node_t* funcVars[c_numOfVars];
    size_t numOfFuncVars;
    size_t numOfFuncFors;

};

struct recInfo_t 
{
    node_t* node;
    funcInfo_t* funcs;
    size_t numOfFuncs;
    size_t funcInd;
};

static void recConvert(recInfo_t* info, FILE** wFile);
static void handleVarsInFunc(recInfo_t* info);
static void handleEquation(recInfo_t* info, FILE** wFile);
static void handleFor(recInfo_t* info, FILE** wFile);
static void handleFunc(recInfo_t* info, FILE** wFile);
static void handleFuncArgs(recInfo_t* info, FILE** wFile);

static void         pasteBaseInfo(const char* basicStartFile, FILE** wFile);
static size_t       findFuncVar(recInfo_t* info, node_t* varNode);
static size_t       addFuncVar   (recInfo_t* info, node_t* varNode);
static const char*  getAsmComparator(types comparator);
static size_t       getFunc(recInfo_t* info);
static void         collectFuncs(recInfo_t* info);
static void         addFunc(recInfo_t* info);
static void         initFuncs(recInfo_t* info);
static void         delFuncs(recInfo_t* info);
static void         handleFuncCall(recInfo_t* info, FILE** wFile);
static const char*  chooseReg(argsStep_t step);
static void         pull_push_Args(argsStep_t step, bool mode, node_t* arg, recInfo_t* info, FILE** wFile);

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

#define GO_DEEPER2_(func)        \
    if (leftNode != nullptr)    \
    {                           \
        info->node = leftNode;  \
        func(info);             \
        info->node = currNode;  \
    }                           \
    if (rightNode != nullptr)   \
    {                           \
        info->node = rightNode; \
        func(info);             \
        info->node = currNode;   \
    }

void writeNASM64(node_t* node, const char* asmFile)
{
    assert(node);
    recInfo_t info = {0};
    info.node = node;
    FILE* wFile = fopen(asmFile, "wb");

    if (wFile == nullptr)
    {
        printf("Error opening file\n");
        return;
    }

    initFuncs(&info);
    collectFuncs(&info);

    printf("%s vars: %s\n", info.funcs[0].func->data.var, 
        info.funcs[0].funcVars[0]->data.var);
    recConvert(&info, &wFile);

    delFuncs(&info);
    fclose(wFile);
}

static void handleFor(recInfo_t* info, FILE** wFile)
{
    node_t* currNode = info->node;

    node_t* initFor = currNode->left;
    node_t* bodyFor = currNode->right;

    node_t* limits = initFor->left;
    node_t* stepCondition = initFor->right;

    node_t* iterator = limits->left->left;
    int start = (int)limits->left->right->data.num;
    int end = (int)limits->right->data.num;

    size_t iterOffset_SFrame = findFuncVar(info, iterator) * 4;
    size_t forInd = info->funcs->numOfFuncFors;

    fprintf(*wFile, "; [comment] for has been started\n\t");

    fprintf(*wFile, "mov  dword [rbp - %lu], %d\n\tjmp CL_for%lu\n\nIL_for%lu:\n\t", 
        iterOffset_SFrame, start, forInd, forInd);
    
    fprintf(*wFile, "; [comment] for_body\n\t");
    info->node = bodyFor;
    recConvert(info, wFile);
    info->node = currNode;
    
    fprintf(*wFile, "; [comment] for_step\n\t");
    // how iterator wil be changing
    info->node = stepCondition;
    recConvert(info, wFile);
    info->node = currNode;
    
    fprintf(*wFile, "; [comment] for_condition\n\t");
    fprintf(*wFile, "\nCL_for%lu:\n\tcmp dword [rbp - %lu], %d\n\tjl IL_for%lu\n\t", 
        forInd, iterOffset_SFrame, end, forInd);
    fprintf(*wFile, "; [comment] for has been ended\n\t");

    ++info->funcs->numOfFuncFors;
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
    case ND_FOR:
    {
        handleFor(info, wFile);
        break;
    }
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
        handleFuncCall(info, wFile);

        break;
    }
    case ND_FUN:
    {
        info->funcInd = getFunc(info);
        printf("ND_FUN %lu\n", info->funcInd);
        
        handleFunc(info, wFile);

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

static void handleFuncCall(recInfo_t* info, FILE** wFile)
{
    assert(info);
    assert(wFile);

    const char* callFuncName = info->node->data.var;

    node_t* arg = info->node->left;
    argsStep_t step = e_edi;

    while (arg)
    {
        pull_push_Args(step, 1, arg, info, wFile);

        step = (argsStep_t)((size_t)step + 1);
        arg = arg->left;
    }
    fprintf(*wFile, "call %s\n\t", callFuncName);
}

static void pull_push_Args(argsStep_t step, bool mode, node_t* arg, recInfo_t* info, FILE** wFile)
{
    types argType = arg->type;
    // push mode = 1
    if (mode)
    {
        if (step < e_stack)
        {
            if (argType == ND_VAR)
            {
                fprintf(*wFile, "mov %s, [rbp - %lu]\n\t", chooseReg(step), findFuncVar(info, arg) * 4);
            }
            else if (argType == ND_NUM)
            {
                int num = (int)arg->data.num;
                fprintf(*wFile, "mov %s, %d\n\t", chooseReg(step), num);
            }
        }
        else if (step >= e_stack)
        {
            if (argType == ND_VAR)
            {
                fprintf(*wFile, "mov rax, [rbp - %lu]\n\tpush rax\n\t", findFuncVar(info, arg) * 4);
            }
            else if (argType == ND_NUM)
            {
                int num = (int)arg->data.num;
                fprintf(*wFile, "push %d\n\t", num);
            }
        }
    }

    // pull mode = 0
    else
    {
        size_t argOffsetStackFrame = findFuncVar(info, arg) * 4;

        if (step < e_stack)
        {
            fprintf(*wFile, "mov [rbp - %lu], %s\n\t", argOffsetStackFrame, chooseReg(step));
        }
        else if (step >= e_stack)
        {
            fprintf(*wFile, "mov rax, [rbp + %lu]\n\tmov [rbp - %lu], rax\n\t", 8 + ((size_t)step - numOfCallerRegs - 1) * 4, argOffsetStackFrame);
        }
    }
}

static const char* chooseReg(argsStep_t step)
{
    switch (step)
    {
    case e_edi: return "edi";
    case e_esi: return "esi";
    case e_edx: return "edx";
    case e_ecx: return "ecx";
    case e_r8d: return "r8d";
    case e_r9d: return "r9d";

    default: return nullptr;
    }
}

static void collectFuncs(recInfo_t* info)
{
    assert(info);

    node_t* currNode = info->node;
    types type = currNode->type;
    node_t* leftNode = currNode->left;
    node_t* rightNode = currNode->right;

    if (type == ND_FUN)
    {
        addFunc(info);

        GO_DEEPER2_(collectFuncs)

        ++info->funcInd;
    }
    else if (type == ND_VAR)
    {
        findFuncVar(info, info->node);

        GO_DEEPER2_(collectFuncs)
    }
    else
    {
        GO_DEEPER2_(collectFuncs)
    }

}

static void initFuncs(recInfo_t* info)
{
    assert(info);

    info->funcInd = 0;
    info->numOfFuncs = 1;
    info->funcs = (funcInfo_t*)calloc(info->numOfFuncs, sizeof(funcInfo_t));
    info->funcs[0].numOfFuncVars = 0;
    info->funcs[0].numOfFuncFors = 0;
}

static void delFuncs(recInfo_t* info)
{
    assert(info);

    info->funcInd = 0;
    info->numOfFuncs = 0;

    free(info->funcs);

    info->funcs = nullptr;
    info->node = nullptr;
}

static void handleFunc(recInfo_t* info, FILE** wFile)
{
    node_t* currNode = info->node;
    
    node_t* funcBody = currNode->right;

    //check main
    if (strcmp("main", currNode->data.var) == 0)
    {
        pasteBaseInfo("../base.ASM", wFile);
        fprintf(*wFile, "_start:\n\tpush rbp\n\tmov rbp, rsp\n\t");
    }
    else
    {
        fprintf(*wFile, "\n%s:\n\tpush rbp\n\tmov rbp, rsp\n\t", currNode->data.var);
    }

    // handleVarsInFunc(info);

    size_t funcInd = info->funcInd;
    funcInfo_t* funcs = info->funcs;
    size_t numOfFuncVars = funcs[funcInd].numOfFuncVars;

    fprintf(*wFile, "sub rsp, %lu\n\t", (numOfFuncVars / 16 + 1) * 16);

    handleFuncArgs(info, wFile);

    info->node = funcBody;
    recConvert(info, wFile);
    info->node = currNode;

    if (strcmp("main", currNode->data.var) == 0)
    {
        fprintf(*wFile, "FINISH\n\t");
    }
    else
    {
        fprintf(*wFile, "mov rsp, rbp\n\tpop rbp\n\tret\n\t");
    }
}

static void handleFuncArgs(recInfo_t* info, FILE** wFile)
{
    node_t* currNode = info->node;
    if (currNode == nullptr) return;

    node_t* arg = currNode->left;
    argsStep_t step = e_edi;

    while (arg)
    {
        pull_push_Args(step, 0, arg, info, wFile);
        
        step = (argsStep_t)((size_t)step + 1);
        arg = arg->left;
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
        fprintf(stderr, "Error opening basicStartFile");
        return;
    }

    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    if (fileSize < 0)
    {
        fprintf(stderr, "Error determining file size");
        fclose(inputFile);
        return;
    }

    char* buffer = (char*)calloc((size_t)fileSize, sizeof(char));
    if (buffer == nullptr)
    {
        fprintf(stderr, "Error allocating buffer");
        fclose(inputFile);
        return;
    }

    size_t bytesRead = fread(buffer, 1, (size_t)fileSize, inputFile);
    if (bytesRead != (size_t)fileSize)
    {
        fprintf(stderr, "Error reading basicStartFile");
        free(buffer);
        fclose(inputFile);
        return;
    }

    size_t bytesWritten = fwrite(buffer, 1, (size_t)fileSize, *wFile);
    if (bytesWritten != (size_t)fileSize)
    {
        fprintf(stderr, "Error writing to wFile");
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

    case ND_PRADD:
    {
        GO_DEEPER_(handleEquation)

        fprintf(*wFile, "pop rax\n\tinc rax\n\tpush rax\n\t");

        break;
    }

    case ND_PRSUB:
    {
        GO_DEEPER_(handleEquation)

        fprintf(*wFile, "pop rax\n\tdec rax\n\tpush rax\n\t");

        break;
    }

    case ND_ISEQ:
    case ND_NISEQ:
    case ND_AB:
    case ND_ABE:
    case ND_LS:
    case ND_LSE:
    {
        GO_DEEPER_(handleEquation)

        fprintf(*wFile, "pop rax\n\tpop rbx\n\t%smovzx rax, al\n\t push rax\n\t", 
                getAsmComparator(currNodeType));

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

static const char* getAsmComparator(types comparator)
{
    switch (comparator)
    {
    // ==, !=, >, >=, <, <=
    case ND_ISEQ:   return "cmp rax, rbx\n\tsete al\n\t";
    case ND_NISEQ:  return "cmp rax, rbx\n\tsetne al\n\t";
    case ND_AB:     return "cmp rax, rbx\n\tsetg al\n\t";
    case ND_ABE:    return "cmp rax, rbx\n\tsetge al\n\t";
    case ND_LS:     return "cmp rax, rbx\n\tsetl al\n\t";
    case ND_LSE:    return "cmp rax, rbx\n\tsetle al\n\t";
    
    // |, &, xor
    case ND_BITAND: return "and rax, rbx\n\tsetnz al\n\t";
    case ND_BITOR:  return "or rax, rbx\n\tsetnz al\n\t";
    case ND_XOR:    return "xor rax, rbx\n\tsete al\n\t";

    default:
        return "???";
    }
}

#pragma GCC diagnostic pop

static size_t getFunc(recInfo_t* info)
{
    size_t numOfFuncs = info->numOfFuncs;
    funcInfo_t* funcs = info->funcs;
    node_t* currFunc = info->node;
    const char* currFuncName =  currFunc->data.var;

    for (size_t i = 0; i < numOfFuncs; ++i)
    {
        if (funcs[i].func == nullptr) break;

        const char* foundedFuncName = funcs[i].func->data.var;
        if (strcmp(foundedFuncName, currFuncName) == 0)
        {
            return i;
        }
    }
    return numOfFuncs;
}

static void addFunc(recInfo_t* info)
{
    assert(info);

    node_t* func = info->node;
    size_t numOfFuncs = info->numOfFuncs;
    size_t funcInd = info->funcInd;

    if (funcInd >= numOfFuncs)
    {
        size_t newNumOfFuncs = numOfFuncs * 2;

        info->funcs = (funcInfo_t*)realloc(info->funcs, newNumOfFuncs * sizeof(funcInfo_t));
        memset(info->funcs + numOfFuncs, 0, (newNumOfFuncs - numOfFuncs) * sizeof(funcInfo_t));

        info->numOfFuncs = newNumOfFuncs;
    }

    funcInfo_t* funcSell = info->funcs + funcInd;

    funcSell->func = func;
}

static size_t findFuncVar(recInfo_t* info, node_t* varNode)
{
    assert(info);
    assert(varNode);

    char* nameOfCurrVar = varNode->data.var;

    size_t funcId = info->funcInd;

    funcInfo_t currFunc = info->funcs[funcId];

    node_t** funcVars = currFunc.funcVars;
    size_t numOfFuncVars = currFunc.numOfFuncVars;

    for (size_t varIndex = 0; varIndex < numOfFuncVars; ++varIndex)
    {

        char* funcVar = funcVars[varIndex]->data.var;

        if (strcmp(funcVar, nameOfCurrVar) == 0) return varIndex + 1;
    }
    return addFuncVar(info, varNode) + 1;
}

static size_t addFuncVar(recInfo_t* info, node_t* varNode)
{
    assert(info);
    assert(varNode);

    size_t funcId = info->funcInd;

    funcInfo_t* currFunc = info->funcs + funcId;

    node_t** funcVars = (*currFunc).funcVars;

    for (size_t i = 0; i < c_numOfVars; ++i)
    {
        if (funcVars[i] == nullptr)
        {
            funcVars[i] = varNode;
            ++(*currFunc).numOfFuncVars;
            return i;
        }
    }
    return (*currFunc).numOfFuncVars;
}


