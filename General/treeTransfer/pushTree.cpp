#include <stdio.h>
#include "../programTree/tree.h"
#include "treeTransfer.h"

static void pushTreeByRecursion(node_t* node, FILE* file);

void pushTree(node_t* node, const char* transferFileName)
{
    FILE* wFile = fopen(transferFileName, "w");

    if (wFile == nullptr)
    {
        printf("Error opening file\n");
        return;
    }

    pushTreeByRecursion(node, wFile);

    fclose(wFile);
    
}

void pushTreeByRecursion(node_t* node, FILE* file)
{
    if (node->type == ND_VAR || node->type == ND_FUN || node->type == ND_FUNCALL || node->type == ND_ENDFOR)
        fprintf(file, "%d %s %d %d\n", node->type, node->data.var->str,
        node->left != nullptr ? 1 : 0, node->right != nullptr ? 1 : 0);
    else if (node->type == ND_NUM)
        fprintf(file, "%d %lg %d %d\n", node->type, node->data.num,
        node->left != nullptr ? 1 : 0, node->right != nullptr ? 1 : 0);
    else
        fprintf(file, "%d %s %d %d\n", node->type, "0",
        node->left != nullptr ? 1 : 0, node->right != nullptr ? 1 : 0);
    
    if (node->left != nullptr)
        pushTreeByRecursion(node->left, file);
    if (node->right != nullptr)
        pushTreeByRecursion(node->right, file);
}
