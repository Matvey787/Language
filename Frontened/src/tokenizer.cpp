#include <cstdlib> 
#include <cctype>  
#include <cstring> 
#include <cstdio>  
#include <assert.h>

#include "tokens.h"

#include "tokenizer.h"


static types findTokenType     (char* word, size_t wordLen);
static void  createToken       (char* word, size_t wordLen, types tokenType, toksInfo_t* tokens);
static bool  handleSpecialWord (char** buffer, size_t buffResidue, toksInfo_t* toksInfo);
static bool  handleNumber      (char** buffer, toksInfo_t* tokens);
static bool  handleVariable    (char** buffer, toksInfo_t* tokens);

node_t* tokenize(char* buffer, size_t len, size_t* toksAmountStorage)
{
    assert(buffer);

    const char* buffEnd = buffer + len;

    toksInfo_t toksInfo = {0};
    toksInfo.tokens  = (node_t*)calloc(1, sizeof(node_t));
    toksInfo.ind = 0;
    toksInfo.cap = 1;

    while (true)
    {
        if (buffer >= buffEnd) break;

        while (*buffer == ' ')  ++buffer;

        size_t buffResidue = (size_t)(buffEnd - buffer);
        bool isFounded = 0;

        
        isFounded = handleSpecialWord(&buffer, buffResidue, &toksInfo);

        
        if (!isFounded) isFounded = handleNumber(&buffer, &toksInfo);

        
        if (!isFounded) isFounded = handleVariable(&buffer, &toksInfo);

        if (!isFounded)
        {
            fprintf(stderr, "Error! Buffer: '%c%c%c' %lu\n", *(buffer-1), *buffer, *(buffer+1), buffResidue);
            for (size_t j = 0; j < toksInfo.ind; j++)
            {
                if (toksInfo.tokens[j].type == ND_VAR)
                {
                    free(toksInfo.tokens[j].data.var);
                }
            }
            free(toksInfo.tokens);
            return nullptr;
        }
    }
    createToken(nullptr, 0, ND_EOT, &toksInfo);

    *toksAmountStorage = toksInfo.ind;
    return toksInfo.tokens;
}

static bool handleSpecialWord(char** buffer, size_t buffResidue, toksInfo_t* toksInfo)
{
    assert(buffer);
    assert(toksInfo);

    for (size_t i = buffResidue > 6 ? 6 : buffResidue; i >= 1; --i)
    {
        types type = ND_ERR;
        if ((type = findTokenType(*buffer, i)) != ND_ERR)
        {
            createToken(*buffer, i, type, toksInfo);
            *buffer += i;
            return true;
        }
    }
    return false;
}

static bool handleNumber(char** buffer, toksInfo_t* tokens)
{
    size_t numberLen = 0;
    char* startOfNum = *buffer;
    while (isdigit(**buffer))
    {
        ++numberLen;
        ++*buffer;
    }

    if (numberLen > 0)
    {
        createToken(startOfNum, numberLen, ND_NUM, tokens);
        return true;
    }
    else
    {
        return false;
    }
}

static bool handleVariable(char** buffer, toksInfo_t* tokens)
{
    
    if (!isalpha(**buffer)) return false;

    size_t varLen = 0;
    char* startOfVar = *buffer;

    while (isalpha(**buffer) || isdigit(**buffer))
    {
        ++varLen;
        ++*buffer;
    }

    createToken(startOfVar, varLen, ND_VAR, tokens);
    return true;

}

static types findTokenType(char* word, size_t wordLen)
{
    assert(word);

    for (size_t i = 0; i < c_token_mapLength - 1; i++)
    {
        size_t tokenLen = token_map[i].len;
        const char* tokenView = token_map[i].str;
        types tokenType = token_map[i].type;

        if (tokenLen < wordLen) break;
        if (tokenLen > wordLen) continue;

        if (strncmp(word, tokenView, tokenLen) == 0) return tokenType;
    }

    return ND_ERR;
}

static void createToken(char* word, size_t wordLen, types tokenType, toksInfo_t* tokens)
{
    size_t index = tokens->ind;
    size_t capacity = tokens->cap;

    if (index >= capacity)
    {
        capacity = tokens->cap * 2;
        tokens->tokens = (node_t*)realloc(tokens->tokens, capacity * sizeof(node_t));
        tokens->cap = capacity;
    }

    tokens->tokens[index].type = tokenType;
    tokens->tokens[index].left = nullptr;
    tokens->tokens[index].right = nullptr;

    if (tokenType == ND_VAR)
    {
        char* str = (char*)calloc(wordLen + 1, sizeof(char));
        strncpy(str, word, wordLen);
        str[wordLen] = '\0';

        tokens->tokens[index].data.var = str;
    }

    if (tokenType == ND_NUM)
    {
        char temp[256];
        strncpy(temp, word, wordLen);
        temp[wordLen] = '\0';
        tokens->tokens[index].data.num = strtod(temp, nullptr);
    }

    ++tokens->ind;
}

void freeTokens(struct node_t* tokens, size_t toksQuantity)
{
    if (tokens) {
        for (size_t i = 0; i < toksQuantity; i++)
        {
            if (tokens[i].type == ND_VAR && tokens[i].data.var)
            {
                free(tokens[i].data.var);
            }
        }
        free(tokens);
    }
}

void drawTokens(struct node_t* tokens, size_t count, const char* directory, const char* filename) {
    assert(tokens);
    assert(directory);
    assert(filename);
    
    char dotPath[256] = {0};
    char svgPath[256] = {0};
    snprintf(dotPath, sizeof(dotPath), "%s/%s.dot", directory, filename);
    snprintf(svgPath, sizeof(svgPath), "%s/%s.svg", directory, filename);
    
    FILE* dotFile = fopen(dotPath, "w");
    assert(dotFile);
    
    fprintf(dotFile, "digraph Tokens {\n");
    fprintf(dotFile, "    bgcolor=\"black\";\n");
    fprintf(dotFile, "    rankdir=TB;\n"); 
    fprintf(dotFile, "    node [shape=box, color=yellow, fontcolor=yellow];\n");
    fprintf(dotFile, "    edge [style=invis];\n"); 

    for (size_t i = 0; i < count; i++) {
        const struct node_t* token = &tokens[i];
        const char* tokenStr = nullptr;
        types tokenType = token->type;

        for (size_t j = 0; token_map[j].str; j++) {
            if (token_map[j].type == tokenType) {
                tokenStr = token_map[j].str;
                break;
            }
        }

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wswitch-enum"

        if (!tokenStr || strcmp(tokenStr, "\n") == 0)
        {
            switch (tokenType)
            {
                case ND_SEP: tokenStr = "newline" ; break;
                case ND_NUM: tokenStr = "number"  ; break;
                case ND_VAR: tokenStr = "variable"; break;
                case ND_EOT: tokenStr = "EOT"     ; break;
                default    : tokenStr = "unknown" ; break;
            }
        }

        #pragma GCC diagnostic pop

        
        char label[256];
        if (tokenType == ND_NUM) {
            snprintf(label, sizeof(label), "\"%s\\nValue: %f\"", tokenStr, token->data.num);
        } else if (tokenType == ND_VAR) {
            snprintf(label, sizeof(label), "\"%s\\nName: %s\"", tokenStr, token->data.var);
        } else {
            snprintf(label, sizeof(label), "\"%s\\nType: %d\"", tokenStr, tokenType);
        }
        
        fprintf(dotFile, "    node%zu [label=%s];\n", i, label);

        
        if (i > 0) {
            fprintf(dotFile, "    node%zu -> node%zu;\n", i - 1, i);
        }
    }

    
    fprintf(dotFile, "    { rank=same; ");
    for (size_t i = 0; i < count; i++) {
        fprintf(dotFile, "node%zu; ", i);
    }
    fprintf(dotFile, "}\n");

    
    fprintf(dotFile, "}\n");
    fclose(dotFile);

    
    char command[1024] = {0};
    snprintf(command, sizeof(command), "dot -Tsvg %s -o %s", dotPath, svgPath);
    system(command);
}