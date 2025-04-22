#ifndef TREE
#define TREE

#include "../errors.h"

/// @brief types of nodes in programm tree
enum types 
{
    ND_ADD=1,
    ND_SUB=2,
    ND_DIV=3,
    ND_MUL=4,
    ND_NUM=5,
    ND_VAR=6,
    ND_POW=7,
    ND_SIN=8,
    ND_COS=9,
    ND_LOG=10,
    ND_SQRT=11,
    
    ND_LCIB=100,
    ND_RCIB=101,
    ND_LCUB=106,
    ND_RCUB=107,

    ND_EOT=102,
    ND_IF=103,
    ND_EQ=104,
    ND_FOR=105,

    ND_SEP = 108,
    ND_POADD = 109,
    ND_ISEQ = 110,
    ND_NISEQ = 111,
    ND_LS = 112,
    ND_AB = 113,
    ND_LSE = 114,
    ND_ABE = 115,
    ND_ENDFOR = 117,
    ND_PR = 118,
    ND_FUN = 119,
    ND_RET = 120,
    ND_FUNCALL = 121,
    ND_GET = 122,
    ND_GETDIFF = 123
};

struct nameTable_t
{
    char* str;
    size_t numOfSymbols;
};

union data_u
{
    double num;
    nameTable_t* var;
};

struct node_t
{
    types type;
    data_u data;
    node_t* left;
    node_t* right;
};

/**
 * @brief Function get node_l and node_r. 
 * Then new head (node) is formed from node_l and node_r (node_l go to head left daughter, node_r go to head right daughter).
 * 
 * @param node A pointer to a tree which should be written to a dot file.
 * @param file_name The path to the file to be write.
 * 
 * @return New node
 */

node_t* newNode(types type, data_u data, node_t* node_l, node_t* node_r);

/**
 * @brief Fuction just make copy of getted node.
 * 
 * @param node Node to be copied
 * 
 * @return Copied node
 */

node_t* copyNode(node_t* node);

/**
 * @brief Function get node and returns a copy of the subtree starting from the one passed node.
 * 
 * @param node Node from which the subtree wil be copied
 * 
 * @return Copied subtree
 */

node_t* copySubtree(node_t* node);
error delTree(node_t* node);

#endif