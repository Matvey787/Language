#include "../../General/programTree/tree.h"
#include "../../General/graphDump/graphDump.h"
#include <stdio.h>
#include <assert.h>
#include "predprocessor.h"

static void problemOccured(const char* func, const char* message = nullptr);

static node_t* getGeneral           (node_t** nodes);
static node_t* getNewLine           (node_t** nodes);
static node_t* chooseContext        (node_t** nodes);
static node_t* getFunc              (node_t** nodes);
static node_t* getInit              (node_t** nodes);
static node_t* getBody              (node_t** nodes);
static node_t* getAppropriation     (node_t** nodes);
static node_t* getAdd               (node_t** nodes);
static node_t* getSub               (node_t** nodes);
static node_t* getMul               (node_t** nodes);
static node_t* getDiv               (node_t** nodes);
static node_t* getSubmodule         (node_t** nodes);
static node_t* chooseOperand        (node_t** nodes);
static node_t* getNum               (node_t** nodes);
static node_t* getVar               (node_t** nodes);
static node_t* getGet               (node_t** nodes);
static node_t* chooseAppropCompar   (node_t** nodes);
static node_t* getComparation       (node_t** nodes);
static node_t* getCompare           (node_t** nodes);
static node_t* getIf                (node_t** nodes);
static node_t* getElse(node_t** nodes);
static node_t* getFor               (node_t** nodes);
static node_t* getWhile(node_t** nodes);
static node_t* getDoWhile(node_t** nodes);
static node_t* getBitAnd(node_t** nodes);
static node_t* getBitOr(node_t** nodes);
static node_t* getXor(node_t** nodes);
static node_t* getPrAdd(node_t** nodes);
static node_t* getPrSub(node_t** nodes);
static node_t* getFunCall(node_t** nodes);
static node_t* getPrint(node_t** nodes);

node_t* createTree(node_t* tokens)
{
    assert(tokens != nullptr);

    node_t* tree = getGeneral(&tokens);
    writeDotFile(tree, "../dot_files/frontenedDotFile.dot");
    writePngFile("../dot_files/frontenedDotFile.dot", "../png_files", "white");
    return tree;
}

static node_t* getGeneral(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* subtree = getNewLine(nodes);

    if ((*nodes)->type != ND_EOT) problemOccured(__func__);

    return subtree;
}

static node_t* getNewLine(node_t** nodes)
{
    assert(nodes != nullptr);

    if ((*nodes)->type == ND_SEP)
    {
        fprintf(stderr, "getBody if: %d\n", (int)((*nodes)->type));
        ++(*nodes); // skip \n
    }

    node_t* l_subtree = chooseContext(nodes);
    //printf("getNewLine: %d\n", (int)((*(nodes))->type));
    
    while ((*nodes)->type == ND_SEP)
    {
        printf("while\n");

        while ((*nodes)->type == ND_SEP)
        {
            ++(*nodes);
        }
        
        node_t* r_subtree = chooseContext(nodes);

        l_subtree = newNode(ND_SEP, {0}, l_subtree, r_subtree);
    }
    return l_subtree;
}

static node_t* chooseContext(node_t** nodes)
{
    assert(nodes != nullptr);

    types type = (*nodes)->type;

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-enum"

    switch (type)
    {
    case ND_FUN:
    {
        ++(*nodes);
        return getFunc(nodes);
    }
    case ND_FUNCALL:
    {
        ++(*nodes);
        return getFunCall(nodes);
    }
    case ND_PR:
    {
        ++(*nodes);
        return getPrint(nodes);
    }
    case ND_VAR:
    {
        return chooseAppropCompar(nodes);
    }
    case ND_NUM:
    {
        return getComparation(nodes);
    }
    case ND_IF:
    {
        ++(*nodes);
        return getIf(nodes);
    }
    case ND_EL:
    {
        ++(*nodes);
        return getElse(nodes);
    }
    case ND_FOR:
    {
        ++(*nodes);
        return getFor(nodes);
    }
    case ND_WH:
    {
        ++(*nodes);
        return getWhile(nodes);
    }
    case ND_DOWH:
    {
        ++(*nodes);
        return getDoWhile(nodes);
    }
    default:
    {
        break;
    }
    }

    #pragma GCC diagnostic pop

    return nullptr;
}

static node_t* getFunCall(node_t** nodes)
{
    assert(nodes);

    // it is function name
    node_t* resultSubtree = getVar(nodes);
    resultSubtree->type = ND_FUNCALL; // ND_VAR -> ND_FUN

    if ((*nodes)->type != ND_LCIB) problemOccured(__func__);

    ++(*nodes);

    node_t* headOfResultSubtree = resultSubtree;

    while ((*nodes)->type == ND_VAR || (*nodes)->type == ND_NUM)
    {
        resultSubtree->left = chooseOperand(nodes);

        resultSubtree = resultSubtree->left;
    }

    if ((*nodes)->type != ND_RCIB) problemOccured(__func__);

    ++(*nodes);

    return headOfResultSubtree;
}

static node_t* getPrint(node_t** nodes)
{
    assert(nodes != nullptr);
    
    if ((*nodes)->type != ND_RCIB) problemOccured(__func__, "( not founded");
    ++(*nodes); // skip (

    node_t* arg = chooseOperand(nodes);
    
    if ((*nodes)->type != ND_RCIB) problemOccured(__func__, ") not founded");
    ++(*nodes); // skip )

    return newNode(ND_PR, {0}, arg, nullptr);

}

static node_t* getIf(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* ifClause = chooseAppropCompar(nodes);
    node_t* ifBody = getBody(nodes);

    return newNode(ND_IF, {0}, ifClause, ifBody);
}

static node_t* getElse(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* elseBody = getBody(nodes);

    return newNode(ND_EL, {0}, nullptr, elseBody);
}

static node_t* getWhile(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* whileClause = chooseAppropCompar(nodes);
    node_t* whileBody = getBody(nodes);

    return newNode(ND_WH, {0}, whileClause, whileBody);
}

static node_t* getDoWhile(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* whileBody = getBody(nodes);
    node_t* whileClause = chooseAppropCompar(nodes);

    return newNode(ND_DOWH, {0}, whileClause, whileBody);
}

static node_t* getFor(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* start = getAppropriation(nodes);

    if ((*nodes)->type != ND_FORDD) problemOccured(__func__);
    ++(*nodes);

    node_t* end = chooseOperand(nodes);
    node_t* forExpr = getAppropriation(nodes);

    node_t* forBody = getBody(nodes);

    return newNode(ND_FOR, {0}, 
           newNode(ND_SEP, {0}, 
           newNode(ND_SEP, {0}, start, end), forExpr), forBody);
}

static node_t* getFunc(node_t** nodes)
{
    assert(nodes != nullptr);
    
    // it is function name
    node_t* resultSubtree = getVar(nodes);
    resultSubtree->type = ND_FUN; // ND_VAR -> ND_FUN

    node_t* l_subtree = getInit(nodes);
    node_t* r_subtree = getBody(nodes);

    resultSubtree->left = l_subtree;
    resultSubtree->right = r_subtree;

    return resultSubtree;
}

static node_t* getInit(node_t** nodes)
{
    assert(nodes != nullptr);

    ++(*nodes);

    node_t* resultSubtree = getVar(nodes);
    
    node_t* resultSubtreeHead = resultSubtree;

    while ((*nodes)->type != ND_RCIB)
    {
        resultSubtree->left = getVar(nodes);
        resultSubtree = resultSubtree->left;
    }
    ++(*nodes);
    return resultSubtreeHead;
}


static node_t* getBody(node_t** nodes)
{
    assert(nodes != nullptr);

    if ((*nodes)->type == ND_SEP)
    {
        fprintf(stderr, "getBody if: %d\n", (int)((*nodes)->type));
        ++(*nodes); // skip \n
    }

    if ((*nodes)->type != ND_LCUB) problemOccured(__func__, "{ not founded");

    ++(*nodes); // skip {
    // ++(*nodes); // skip \n

    node_t* resultNode = getNewLine(nodes);

    if ((*nodes)->type != ND_RCUB) problemOccured(__func__, "} not founded");

    ++(*nodes); //skip }

    return resultNode;
}

static node_t* chooseAppropCompar(node_t** nodes)
{
    types type = (*nodes + 1)->type;

    if (type == ND_AB || type == ND_ABE || type == ND_LS || type == ND_LSE ||
        type == ND_ISEQ || type == ND_NISEQ)
    {
        return getComparation(nodes);
    }
    else if (type == ND_EQ)
    {
        return getAppropriation(nodes);
    }
    else
    {
        problemOccured(__func__);
        return nullptr;
    }
}

static node_t* getComparation(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = chooseOperand(nodes);
    types type = (*nodes)->type;

    ++(*nodes);

    node_t* r_subtree = getCompare(nodes);

    return newNode(type, {0}, l_subtree, r_subtree);
}

static node_t* getAppropriation(node_t** nodes)
{
    printf("getAppropriation: %d\n", (*nodes)->type);
    assert(nodes != nullptr);

    node_t* l_subtree = getVar(nodes);

    ++(*nodes);

    node_t* r_subtree = getPrAdd(nodes);

    return newNode(ND_EQ, {0}, l_subtree, r_subtree);
}

static node_t* getPrAdd(node_t** nodes)
{
    assert(nodes != nullptr);
    
    types type = (*nodes)->type;
    if (type == ND_PRADD)
    {
        ++(*nodes);
    }
    node_t* operand = getPrSub(nodes);

    if (type == ND_PRADD)
    {
        operand = newNode(ND_PRADD, {0}, operand, nullptr);
    }

    return operand;
}

static node_t* getPrSub(node_t** nodes)
{
    assert(nodes != nullptr);
    
    types type = (*nodes)->type;
    if (type == ND_PRSUB)
    {
        ++(*nodes);
    }
    node_t* operand = getCompare(nodes);

    if (type == ND_PRSUB)
    {
        operand = newNode(ND_PRSUB, {0}, operand, nullptr);
    }

    return operand;
}

static node_t* getCompare(node_t** nodes)
{
    assert(nodes != nullptr);
    printf("getCompare: %d\n", (*nodes)->type);
    node_t* l_subtree = getBitAnd(nodes);
    types type = (*nodes)->type;
    printf("getCompare2: %d\n", (*nodes)->type);

    while ((*nodes)->type == ND_AB || (*nodes)->type == ND_ABE || (*nodes)->type == ND_LS ||
           (*nodes)->type == ND_LSE || (*nodes)->type == ND_ISEQ || (*nodes)->type == ND_NISEQ)
    {
        ++(*nodes);
        printf("getCompare3: %d\n", (*nodes)->type);
        node_t* r_subtree = getBitAnd(nodes);
        l_subtree = newNode(type, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getBitAnd(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = getBitOr(nodes);

    while ((*nodes)->type == ND_BITAND)
    {
        ++(*nodes);
        node_t* r_subtree = getBitOr(nodes);
        l_subtree = newNode(ND_BITAND, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getBitOr(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = getXor(nodes);

    while ((*nodes)->type == ND_BITOR)
    {
        ++(*nodes);
        node_t* r_subtree = getXor(nodes);
        l_subtree = newNode(ND_BITOR, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getXor(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = getAdd(nodes);

    while ((*nodes)->type == ND_XOR)
    {
        ++(*nodes);
        node_t* r_subtree = getAdd(nodes);
        l_subtree = newNode(ND_XOR, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getAdd(node_t** nodes)
{
    assert(nodes != nullptr);

    printf("getAdd: %d\n", (*nodes)->type);
    node_t* l_subtree = getSub(nodes);

    while ((*nodes)->type == ND_ADD)
    {
        ++(*nodes);
        node_t* r_subtree = getSub(nodes);
        l_subtree = newNode(ND_ADD, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getSub(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = getMul(nodes);

    while ((*nodes)->type == ND_SUB)
    {
        ++(*nodes);
        node_t* r_subtree = getMul(nodes);
        l_subtree = newNode(ND_SUB, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getMul(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = getDiv(nodes);

    while ((*nodes)->type == ND_MUL)
    {
        ++(*nodes);
        node_t* r_subtree = getDiv(nodes);
        l_subtree = newNode(ND_MUL, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getDiv(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* l_subtree = getSubmodule(nodes);

    while ((*nodes)->type == ND_DIV)
    {
        ++(*nodes);
        node_t* r_subtree = getSubmodule(nodes);
        l_subtree = newNode(ND_DIV, {0}, l_subtree, r_subtree);
    }

    return l_subtree;
}

static node_t* getSubmodule(node_t** nodes)
{
    printf("getSubmodule: %d\n", (*nodes)->type);
    assert(nodes != nullptr);

    node_t* resultSubtree = nullptr;

    if ((*nodes)->type == ND_LCIB)
    {
        ++(*nodes);
        resultSubtree = getPrAdd(nodes);
        if ((*nodes)->type != ND_RCIB) problemOccured(__func__);
        ++(*nodes);
    }
    else
    {
        resultSubtree = chooseOperand(nodes);
    }

    return resultSubtree;
}

static node_t* chooseOperand(node_t** nodes)
{
    assert(nodes != nullptr);

    types type = (*nodes)->type;

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-enum"

    switch (type)
    {
    case ND_NUM: return getNum(nodes);
    case ND_VAR: return getVar(nodes);
    case ND_GET: return getGet(nodes);
    default:
        break;
    }
    #pragma GCC diagnostic pop
    return nullptr;
}

static node_t* getNum(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* number = *nodes;
    ++(*nodes);
    return copyNode(number);
}

static node_t* getVar(node_t** nodes)
{
    assert(nodes != nullptr);

    if ((*nodes)->type == ND_VAR)
    {
        node_t* var = *nodes;
        ++(*nodes);
        return copyNode(var);
    }

    return nullptr;
}

//FIXME - no correct init
static node_t* getGet(node_t** nodes)
{
    assert(nodes != nullptr);

    node_t* var = *nodes;
    ++(*nodes);
    return copyNode(var);
}

static void problemOccured(const char* func, const char* message)
{
    fprintf(stderr, "Something go wrong in creating tree... (%s: %s)\n", func, message);
}