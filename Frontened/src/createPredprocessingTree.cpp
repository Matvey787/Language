#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include "../../General/programTree/tree.h"
#include "workWithFile.h"
#include "../../General/graphDump/graphDump.h"
#include "../../General/errors.h"
#include "createPredprocessingTree.h"

// Functions that allow recursive descent and construction of a program tree.
static node_t* getGeneral(node_t** exp);
static node_t* getAddSub(node_t** exp);
static node_t* getMulDiv(node_t** exp);
static node_t* getSubModule(node_t** exp);
static node_t* getAppropriation(node_t** nodes);
static node_t* Num_OR_Var_OR_Get(node_t** exp);
static node_t* getNum(node_t** exp);
static node_t* getVar(node_t** exp);
static node_t* getSin(node_t** exp);
static node_t* getCos(node_t** exp);
static node_t* getLog(node_t** exp);
static node_t* getPOW(node_t** nodes);
static node_t* getNewLine(node_t** nodes);
static node_t* chooseMode(node_t** nodes);
static node_t* getIF(node_t** nodes);
static node_t* getFor(node_t** nodes);
static node_t* getPrint(node_t** nodes);
static node_t* getFunc(node_t** nodes);
static node_t* getReturn(node_t** nodes);
static node_t* getCallOfFunc(node_t** nodes);
static node_t* getSqrt(node_t** nodes);
static node_t* getGet(node_t** nodes);
static node_t* getDiff(node_t** nodes);

// Handles syntax errors by printing an error message and exiting the program.

static void syntaxError();

// Function is used in case ND_IF for creating subtree in if clauses

static node_t* newNode_by_ComparisonOperator(types type, node_t* l_subtree, node_t** nodes);

node_t* createPredprocessingTree(node_t* tokens, const char* dotFile, const char* directory)
{
    assert(tokens != nullptr);
    node_t* tree = getGeneral(&tokens);
    writeDotFile(tree, dotFile);
    writePngFile(dotFile, directory, "white");

    return tree;
}

// main function of recursive descent. Do nothing... just call other functions.
static node_t* getGeneral(node_t** nodes)
{
    assert(nodes != nullptr);
    node_t* subtree = getNewLine(nodes);
    return subtree;
}

// Function check if the current token is a separator and if so, create ND_SEP node.
static node_t* getNewLine(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = chooseMode(nodes);
    
    while ((*nodes)->type == ND_SEP)
    {
        (*nodes)++;
        node_t* r_subtree = chooseMode(nodes);
        
        if (r_subtree != nullptr && r_subtree->type == ND_RCUB)
            break;

        l_subtree = newNode(ND_SEP, {0}, l_subtree, r_subtree);
    }
    return l_subtree;
}

static node_t* chooseMode(node_t** nodes)
{
    assert(nodes != nullptr);

    if ((*nodes)->type == ND_VAR)
    {
        return getAppropriation(nodes);
    }

    else if ((*nodes)->type == ND_IF)
        return getIF(nodes);
    else if ((*nodes)->type == ND_FOR)
    {
        return getFor(nodes);
    }
    else if ((*nodes)->type == ND_PR)
    {
        return getPrint(nodes);
    }
    else if ((*nodes)->type == ND_FUN)
    {
        return getFunc(nodes);
    }
    else if ((*nodes)->type == ND_RET)
    {
        return getReturn(nodes);
    }
    else if ((*nodes)->type == ND_GETDIFF)
    {
        return getDiff(nodes);
    }
    else
        return getAddSub(nodes);
}

static node_t* getFunc(node_t** nodes)
{
    assert(nodes != nullptr);
    *nodes += 1;
    node_t* funcNode = copyNode(*nodes);
    funcNode->type = ND_FUN;
    *nodes += 1;
    node_t* parameters = getSubModule(nodes);
    *nodes += 1;
    node_t* funcBody = getSubModule(nodes);
    funcNode->left = parameters; // create left subtree of parameters in funcNode
    funcNode->right = funcBody; // create right subtree of func body in funcNode
    return funcNode;
}   

static node_t* getDiff(node_t** nodes)
{
    assert(nodes != nullptr);
    node_t* diffNode = copyNode(*nodes);
    *nodes += 1;
    node_t* parameters = getSubModule(nodes);
    diffNode->left = parameters;
    return diffNode;
}

static node_t* getCallOfFunc(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* funcCallNode = copyNode(*nodes);
    funcCallNode->type = ND_FUNCALL; // we change type of node because firstly it was ND_VAR
    *nodes += 1;
    node_t* passedParameters = getSubModule(nodes);
    funcCallNode->left = passedParameters;

    return funcCallNode;
}


static node_t* getFor(node_t** nodes)
{
    assert(nodes != nullptr);
    *nodes += 1;
    node_t* iterator = getVar(nodes);

    if ((*nodes)->type == ND_EQ)
    {
        
        *nodes += 1;
        node_t* start = getNum(nodes);
        node_t* varEndOfFor = Num_OR_Var_OR_Get(nodes);
        node_t* end = getNum(nodes);
        node_t* step = getAddSub(nodes);
        
        *nodes += 1;
        return newNode(ND_FOR, {0}, 
                                    newNode(ND_SEP, {0}, 
                                                        newNode(ND_SEP, {0}, newNode(ND_EQ, {0}, iterator,    start) ,
                                                                             newNode(ND_EQ, {0}, varEndOfFor, end  )),
                                                        step),        
                                    getSubModule(nodes)); 
    }
    else 
    {
        syntaxError();
        return nullptr;
    }
}

static node_t* getIF(node_t** nodes)
{
    assert(nodes != nullptr);
    *nodes += 1;
    node_t* l_subtree = getAddSub(nodes);
    return newNode_by_ComparisonOperator((*nodes)->type, l_subtree, nodes);
}

static node_t* getPrint(node_t** nodes)
{
    assert(nodes != nullptr);
    assert(*nodes != nullptr);
    *nodes += 1;
    node_t* l_subtree = getAddSub(nodes);
    return newNode(ND_PR, {0}, l_subtree, nullptr);
}

static node_t* getReturn(node_t** nodes)
{
    assert(nodes != nullptr);
    assert(*nodes != nullptr);
    *nodes += 1;
    node_t* l_subtree = getAddSub(nodes);
    return newNode(ND_RET, {0}, l_subtree, nullptr);
}

static node_t* newNode_by_ComparisonOperator(types type, node_t* l_subtree, node_t** nodes)
{
    assert(l_subtree != nullptr);
    assert(nodes != nullptr);
    assert(*nodes != nullptr);

    *nodes += 1;
    node_t* r_subtree = getAddSub(nodes);

    *nodes += 1;
    switch (type)
    {
    case ND_ISEQ:
        return newNode(ND_IF, {0}, newNode(ND_ISEQ,  {0}, l_subtree, r_subtree), getSubModule(nodes));
        break;
    case ND_NISEQ:
        return newNode(ND_IF, {0}, newNode(ND_NISEQ, {0}, l_subtree, r_subtree), getSubModule(nodes));
        break;
    case ND_LS:
        return newNode(ND_IF, {0}, newNode(ND_LS,    {0}, l_subtree, r_subtree), getSubModule(nodes));
        break;
    case ND_AB:
        return newNode(ND_IF, {0}, newNode(ND_AB,    {0}, l_subtree, r_subtree), getSubModule(nodes));
        break;
    case ND_LSE:
        return newNode(ND_IF, {0}, newNode(ND_LSE,   {0}, l_subtree, r_subtree), getSubModule(nodes));
        break;
    case ND_ABE:
        return newNode(ND_IF, {0}, newNode(ND_ABE,   {0}, l_subtree, r_subtree), getSubModule(nodes));
        break;
    case ND_SUB:
    case ND_DIV:
    case ND_RET:
    case ND_GET:
    case ND_MUL:
    case ND_NUM:
    case ND_VAR:
    case ND_POW:
    case ND_SIN:
    case ND_COS:
    case ND_LOG:
    case ND_LCIB:
    case ND_RCIB:
    case ND_LCUB:
    case ND_RCUB:
    case ND_FUN:
    case ND_FUNCALL:
    case ND_EOT:
    case ND_IF:
    case ND_EQ:
    case ND_FOR:
    case ND_SEP:
    case ND_POADD:
    case ND_GETDIFF:
    case ND_ADD:
    case ND_ENDFOR:
    case ND_PR:
    case ND_SQRT:
    default:
        return copyNode(*nodes);
        break;
    }
}

static node_t* getAppropriation(node_t** nodes)
{
    assert(nodes != nullptr);
    node_t* l_subtree = getAddSub(nodes);
    
    if ((*nodes)->type == ND_EQ)
    {
        *nodes += 1;
        node_t* r_subtree = getAddSub(nodes);
        return newNode(ND_EQ, {0}, l_subtree, r_subtree);

    }
    return l_subtree;
}

static node_t* getAddSub(node_t** nodes)
{
    assert(nodes != nullptr);
    node_t* l_subtree = getMulDiv(nodes);
    while ((*nodes)->type == ND_ADD || (*nodes)->type == ND_SUB || (*nodes)->type == ND_GET)
    {
        types op = (*nodes)->type;
        (*nodes)++;
        node_t* r_subtree = getMulDiv(nodes);

        if (op == ND_ADD)
            l_subtree = newNode(ND_ADD, {0}, l_subtree, r_subtree);
        else
            l_subtree = newNode(ND_SUB, {0}, l_subtree, r_subtree);
    }
    return l_subtree;
}

static node_t* getMulDiv(node_t** nodes)
{
    assert(nodes != nullptr);
    node_t* l_subtree = getPOW(nodes);
    while ((*nodes)->type == ND_MUL || (*nodes)->type == ND_DIV)
    {
        types op = (*nodes)->type;
        (*nodes)++;
        node_t* r_subtree = getPOW(nodes);

        if (op == ND_MUL)
            l_subtree = newNode(ND_MUL, {0}, l_subtree, r_subtree);
        else
            l_subtree = newNode(ND_DIV, {0}, l_subtree, r_subtree);
    }
    return l_subtree;
}

static node_t* getPOW(node_t** nodes)
{
    assert(nodes != nullptr);
    node_t* l_subtree = getSubModule(nodes);
    while ((*nodes)->type == ND_POW)
    {
        *nodes += 1;
        node_t* r_subtree = getSubModule(nodes);

        l_subtree = newNode(ND_POW, {0}, l_subtree, r_subtree);
    }
    return l_subtree;
}

static node_t* getSubModule(node_t** nodes)
{
    printf("getSubModule %d\n", (*nodes)->type);
    if ((*nodes)->type == ND_LCIB || (*nodes)->type == ND_LCUB)
    {
        node_t* val = nullptr;
        if ((*nodes)->type == ND_LCUB)
        {
            *nodes += 2;
            val = getNewLine(nodes);
        }
        else if ((*nodes)->type == ND_LCIB)
        {
            ++*nodes;
            val = getAddSub(nodes);
        }
        
        if ((*nodes)->type != ND_RCIB && (*nodes)->type != ND_RCUB)
        {
            printf("Hmmm... Something wrong with brackets\n");
            syntaxError();
        }
        *nodes += 1;
        return val;
    }
    else
    {
        return getSin(nodes);
    }
}

static node_t* getSin(node_t** nodes)
{
    if ((*nodes)->type == ND_SIN)
    {
        node_t* sin = *nodes;
        (*nodes)++;
        sin->left = getSubModule(nodes);
        return copyNode(sin);
    }
    else
    {
        return getCos(nodes);
    }   
}

static node_t* getCos(node_t** nodes)
{
    if ((*nodes)->type == ND_COS)
    {
        node_t* cos = *nodes;
        (*nodes)++;
        cos->left = getSubModule(nodes);
        return copyNode(cos);
    }
    else
    {
        return getSqrt(nodes);
    }   
}

static node_t* getSqrt(node_t** nodes)
{
    assert(nodes != nullptr);
    assert(*nodes != nullptr);
    
    if ((*nodes)->type == ND_SQRT)
    {
        node_t* sqrt = *nodes;
        (*nodes)++;
        if ((*nodes)->type != ND_LCIB)
        {
            sqrt->right = Num_OR_Var_OR_Get(nodes);
            sqrt->right->data.num = 1 / sqrt->right->data.num;
            sqrt->type = ND_POW;
        }
        sqrt->left = getSubModule(nodes);
        return copyNode(sqrt);
    }
    else
    {
        return getLog(nodes);
    }
}

// function to get log
static node_t* getLog(node_t** nodes)
{

    if ((*nodes)->type == ND_LOG)
    {
        node_t* log = *nodes;
        (*nodes)++;
        log->right = Num_OR_Var_OR_Get(nodes);
        log->left = getSubModule(nodes);
        return copyNode(log);
    }
    else
    {
        return Num_OR_Var_OR_Get(nodes);
    }
}

// function for understanf if node is number or variable
static node_t* Num_OR_Var_OR_Get(node_t** nodes)
{

    if ((*nodes)->type == ND_NUM)
        return getNum(nodes);
    else if ((*nodes)->type == ND_VAR || (*nodes)->type == ND_ENDFOR)
        return getVar(nodes);
    else if ((*nodes)->type == ND_POADD)
    {
        node_t* poAdd = copyNode(*nodes);
        (*nodes)++;
        return poAdd;
    }
    else if ((*nodes)->type == ND_GET)
        return getGet(nodes);

    else if ((*nodes)->type == ND_GETDIFF)
        return getDiff(nodes);
    else 
        return nullptr;
}

static node_t* getGet(node_t** nodes)
{
    assert(nodes != nullptr);
    assert(*nodes != nullptr);
    (*nodes)++;
    return newNode(ND_GET, {0}, nullptr, nullptr);
}


// function for getting number
static node_t* getNum(node_t** nodes)
{
    node_t* number = *nodes;
    (*nodes)++;
    return copyNode(number);
}

// function for getting variable
static node_t* getVar(node_t** nodes)
{
    if (*nodes + 1 != nullptr && (*nodes + 1)->type == ND_LCIB)
    {
        return getCallOfFunc(nodes);
    }
    node_t* var = *nodes;
    (*nodes)++;
    return copyNode(var);
}

static void syntaxError()
{
    printf("error!\n");
    exit(1);
}
