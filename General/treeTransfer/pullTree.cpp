#include <stdio.h>
#include "../programTree/tree.h"
#include "treeTransfer.h"
#include <stdlib.h> 
#include <string.h>

static void pullTreeByRecursion(node_t** node, FILE** rFile);
// static size_t count_lines_in_file(FILE* file);

node_t* pullTree(const char* transferFileName)
{
    FILE* rFile = fopen(transferFileName, "r");
    int counter = 0;
    int c = 0;
    node_t* mainNode = (node_t*)calloc(1, sizeof(node_t));

    fscanf(rFile, "%d", (int*)&(mainNode->type));

    // first line should be missed 
    while ( c != EOF && counter != 1)
    {
        c = fgetc(rFile);
        if (c == '\n')
            counter++;
    }

    mainNode->data = {0};
    mainNode->left = nullptr;
    mainNode->right = nullptr;

    pullTreeByRecursion(&mainNode->left, &rFile);
    printf("mainNode type %d\n", mainNode->type);
    if (mainNode->type != ND_SEP)
    {
        printf("get right subtree in pullTree");
        pullTreeByRecursion(&mainNode->right, &rFile);
    }

    return mainNode;
}

static void pullTreeByRecursion(node_t** node, FILE** rFile)
{
    static int counter = 0;
    counter++;
    *node = (node_t*)calloc(1, sizeof(node_t));
    fscanf(*rFile, "%d", (int*)&(*node)->type);
    printf("pullTreeByRec %d\n", (*node)->type);

    if ((*node)->type == ND_VAR || (*node)->type == ND_FUN || (*node)->type == ND_ENDFOR
     || (*node)->type == ND_FUNCALL)
    {
        char tempStr[100] = {0};
        fscanf(*rFile, "%s", tempStr);
        
        (*node)->data.var = (char*)calloc(strlen(tempStr) + 1, sizeof(char));

        memcpy((*node)->data.var, tempStr, strlen(tempStr));

        printf("var %s\n", (*node)->data.var);
    }
    else
    {
        fscanf(*rFile, "%lg", &(*node)->data.num);
        printf("num %lg\n", (*node)->data.num);
    }
    

    int existLeftTree = 0;
    fscanf(*rFile, "%d", &existLeftTree);
    int existRightTree = 0;
    fscanf(*rFile, "%d", &existRightTree);

    if (existLeftTree)
    {
        printf("go to left\n");
        pullTreeByRecursion(&(*node)->left, rFile);
    }

    if (existRightTree)
    {
        printf("go to right\n");
        pullTreeByRecursion(&(*node)->right, rFile);
    }

}