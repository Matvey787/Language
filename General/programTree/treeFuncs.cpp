#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "tree.h"
#include "../errors.h"

static error delBranches(node_t* node);

node_t* newNode(types type, data_u data, node_t* node_l, node_t* node_r)
{
    node_t* varNewNode = (node_t*)calloc(1, sizeof(node_t));

    varNewNode->type = type;
    varNewNode->data = data;
    
    varNewNode->left = node_l;
    varNewNode->right = node_r;
    return varNewNode;
}

node_t* copyNode(node_t* node){
    assert(node != nullptr);
    node_t* varCopyNode = (node_t*)calloc(1, sizeof(node_t));
    assert(varCopyNode != nullptr);
    varCopyNode->type = node->type;
    varCopyNode->data = node->data;
    varCopyNode->left = node->left;
    varCopyNode->right = node->right;
    return varCopyNode;
    
}

node_t* copySubtree(node_t* node){
    assert(node != nullptr);
    node_t* varCopyNode = (node_t*)calloc(1, sizeof(node_t));
    
    if (varCopyNode == nullptr)
        printf("allocate memory fail\n");

    varCopyNode->type = node->type;
    varCopyNode->data = node->data;

    if (node->left != nullptr)
    {
        varCopyNode->left = copySubtree(node->left);
    }
    else
        varCopyNode->left = nullptr;

    if (node->right != nullptr)
    {
        varCopyNode->right = copySubtree(node->right);
    }  
    else
        varCopyNode->right = nullptr;
    return varCopyNode;
}

/**
 * @brief Function get node and returns a copy of the subtree starting from the one passed node.
 * 
 * @param node Tree which which should be deleted
 * 
 * @return no error if tree was deleted successfully else error type
 */

error delTree(node_t* node){
    assert(node != nullptr);
    if (node == nullptr)
        return NULLPTR_ERROR;
    fprintf(stderr, "branches after node %p go free\n", node);
    delBranches(node);
    fprintf(stderr, "node %p go free\n", node);
    free(node);

    node = nullptr;
    return NO_ERRORS;
}

// Function need for recursivity delete branches. Used in delTree

static error delBranches(node_t* node){
    printf("curent node: %p left: %p right: %p\n", node, node, node);
    assert(node != nullptr);

    error status = NO_ERRORS;

    if (node == nullptr)
        return NULLPTR_ERROR;

    if (node->left != nullptr)
        status = delBranches(node->left);
    
    if (node->right != nullptr)
        status = delBranches(node->right);

    if (status != NO_ERRORS)
        return status;
        
    if (node->left != nullptr)
    {
        fprintf(stderr, "left node %p go free\n", node->left);
        free(node->left);
        node->left = nullptr;
    }
    
    if (node->right != nullptr)
    {
        fprintf(stderr, "right node %p go free\n", node->right);
        free(node->right);
        node->right = nullptr;
    }
    return status;
}