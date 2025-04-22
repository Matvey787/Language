#include <stdio.h>
#include "../programTree/tree.h"
#include "treeTransfer.h"
#include <stdlib.h> 
#include <string.h>

static void pullTreeByRecursion(node_t** node, nameTable_t** nameTable, FILE** rFile);
// static size_t count_lines_in_file(FILE* file);

node_t* pullTree(nameTable_t** nameTable, const char* transferFileName)
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

    pullTreeByRecursion(&mainNode->left, nameTable, &rFile);
    printf("mainNode type %d\n", mainNode->type);
    if (mainNode->type != ND_SEP)
    {
        printf("get right subtree in pullTree");
        pullTreeByRecursion(&mainNode->right, nameTable, &rFile);
    }

    return mainNode;
}

// static size_t count_lines_in_file(FILE* file) {
//     if (file == NULL) {
//         perror("Error opening file");
//         return 0;
//     }

//     char *line = NULL; // Указатель для строки
//     size_t len = 0;    // Длина буфера
//     size_t lines = 0;

//     while (getline(&line, &len, file) != -1) {
//         lines++;
//     }

//     free(line);  // Освобождаем память, выделенную getline
//     fclose(file);
//     return lines;
// }

static void pullTreeByRecursion(node_t** node, nameTable_t** nameTable, FILE** rFile)
{
    static int counter = 0;
    counter++;
    *node = (node_t*)calloc(1, sizeof(node_t));
    fscanf(*rFile, "%d", (int*)&(*node)->type);
    printf("pullTreeByRec %d\n", (*node)->type);
    if ((*node)->type == ND_VAR || (*node)->type == ND_FUN || (*node)->type == ND_ENDFOR || (*node)->type == ND_FUNCALL/* || (*node)->type == ND_START || (*node)->type == ND_END */)
    {
        char tempStr[100] = {0};
        fscanf(*rFile, "%s", tempStr);
        (*nameTable)->str = (char*)calloc(strlen(tempStr) + 1, sizeof(char));
        memcpy((*nameTable)->str, tempStr, strlen(tempStr));
        (*node)->data.var = *nameTable;
        *nameTable += 1;
        printf("var %s\n", (*node)->data.var->str);
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
        pullTreeByRecursion(&(*node)->left, nameTable, rFile);
    }

    if (existRightTree)
    {
        printf("go to right\n");
        pullTreeByRecursion(&(*node)->right, nameTable, rFile);
    }

}