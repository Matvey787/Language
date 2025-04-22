#include <stdio.h>
#include <assert.h>

#include "graphDump.h"


static void writeTreeToDotFile(node_t* node, FILE** wFile, size_t rank);

void writeDotFile(node_t* node, const char* fileName)
{
    assert(node != nullptr);
    assert(fileName != nullptr);
    
    FILE* wFile = fopen(fileName, "w");
    if (wFile == nullptr){
        printf("couldn't open file");
        return;
    }

    fprintf(wFile, "digraph\n{ \nrankdir=HR;\n");
    fprintf(wFile, "\n");

    writeTreeToDotFile(node, &wFile, 1);

    fprintf(wFile, "}\n");

    fclose(wFile);
}

static void writeTreeToDotFile(node_t* node, FILE** wFile, size_t rank){
    assert(node != nullptr);
    assert(wFile != nullptr);

    switch (node->type)
    {
    case ND_DIV:
    case ND_GET:
    case ND_MUL:
    case ND_POADD:
    case ND_ADD:
    case ND_SUB:
    case ND_POW:
    case ND_SIN:
    case ND_COS:
    case ND_LOG:
    case ND_SQRT:
    case ND_IF:
    case ND_EQ:
    case ND_FOR:
    case ND_EOT:
    case ND_SEP:
    case ND_PR:
    case ND_GETDIFF:
    case ND_RET:
    case ND_ISEQ:
    case ND_NISEQ:
    case ND_AB:
    case ND_LS:
    case ND_ABE:
    case ND_LSE:
    {
        fprintf(*wFile, "node%p [ shape=record, color = %s rank = %lu, label= \"{ %p | %s | \
        {<n%p_l> left | <n%p_r> right}} \" ];\n",
        node, getColor(node->type), rank, node, convertTypeToStr(node->type), node, node);

        break;
    }
    case ND_NUM:
    {
        fprintf(*wFile, "node%p [ shape=record, color = %s rank = %lu, label= \"{ %p | %s | %lg | \
        {<n%p_l> left | <n%p_r> right}} \" ];\n", 
        node, getColor(node->type), rank, node, convertTypeToStr(node->type), node->data.num, node, node);

        break;
    }
    case ND_VAR:
    case ND_ENDFOR:
    case ND_FUNCALL:
    case ND_FUN:
    {
        printf("------> %d\n", node->type);
        assert (node->data.var != nullptr);
        fprintf(*wFile, "node%p [ shape=record, color = %s rank = %lu, label= \"{ %p | %s | %s | \
        {<n%p_l> left | <n%p_r> right}} \" ];\n", 
        node, getColor(node->type), rank, node, convertTypeToStr(node->type), node->data.var->str, node, node);

        break;
    }
    case ND_RCIB:
    case ND_LCIB:
    case ND_LCUB:
    case ND_RCUB:
    default:
        break;
    }

    
    if (node->left != nullptr)
    {
        writeTreeToDotFile(node->left, wFile, ++rank);
        fprintf(*wFile, "node%p:<n%p_l>:s -> node%p:n [ color = black; ]\n", node, node, node->left);
    }
    if (node->right != nullptr)
    {
        writeTreeToDotFile(node->right, wFile, ++rank);
        fprintf(*wFile, "node%p:<n%p_r>:s -> node%p:n [ color = black; ]\n", node, node, node->right);
    }
}