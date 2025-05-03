#include <cstdlib> 
#include <cctype>  
#include <cstring> 
#include <cstdio>  
#include <assert.h>

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
    ND_PRADD = 109,
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
    ND_GETDIFF = 123,
    ND_FORDD = 124,

    ND_WH = 125,
    ND_DOWH = 126,
    ND_EL = 127,
    ND_PRSUB = 128,
    ND_AND = 129,
    ND_OR = 130,
    ND_BITAND = 131,
    ND_BITOR = 132,
    ND_XOR = 133,

    ND_ERR = 999
};

union data_u
{
    double num;
    char* var;
};

struct node_t
{
    types type;
    data_u data;
    node_t* left;
    node_t* right;
};

struct token_map_t {
    const char* str;
    size_t len;
    enum types type;
    
};

const size_t c_token_mapLength = 40;
static const token_map_t token_map[] = {
    {"return", 6, ND_RET},
    {"while",  5, ND_WH},
    {"print",  5, ND_PR},
    {"get()",  5, ND_GET},
    {"else",   4, ND_EL},
    {"sqrt",   4, ND_SQRT},
    {"func",   4, ND_FUN},
    {"diff",   4, ND_GETDIFF},
    {"for",    3, ND_FOR},
    {"sin",    3, ND_SIN},
    {"cos",    3, ND_COS},
    {"log",    3, ND_LOG},
    {"do",     2, ND_DOWH},
    {"..",     2, ND_FORDD},
    {"if",     2, ND_IF},
    {"==",     2, ND_ISEQ},
    {"!=",     2, ND_NISEQ},
    {"++",     2, ND_PRADD},
    {"--",     2, ND_PRSUB},
    {"&&",     2, ND_AND},
    {"||",     2, ND_OR},
    {"<=",     2, ND_LSE},
    {">=",     2, ND_ABE},
    {"&",      1, ND_BITAND},
    {"|",      1, ND_BITOR},
    {"^",      1, ND_XOR},
    {"+",      1, ND_ADD},
    {"-",      1, ND_SUB},
    {"*",      1, ND_MUL},
    {"/",      1, ND_DIV},
    {"(",      1, ND_LCIB},
    {")",      1, ND_RCIB},
    {"{",      1, ND_LCUB},
    {"}",      1, ND_RCUB},
    {"^",      1, ND_POW},
    {"=",      1, ND_EQ},
    {"<",      1, ND_LS},
    {">",      1, ND_AB},
    {"\n",     1, ND_SEP},
    {nullptr,  0, ND_ERR}
};

struct toksInfo_t
{
    node_t* tokens;
    size_t ind;
    size_t cap;
};

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
            fprintf(stderr, "Error! Buffer: %c %lu\n", *buffer, buffResidue);
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