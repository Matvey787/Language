#ifndef TOKENIZER_H
#define TOKENIZER_H

node_t* tokenize(char* buffer, size_t len, size_t* toksAmountStorage);
void freeTokens(struct node_t* tokens, size_t toksQuantity);
void drawTokens(struct node_t* tokens, size_t count, const char* directory, const char* filename);

#endif // TOKENIZER_H