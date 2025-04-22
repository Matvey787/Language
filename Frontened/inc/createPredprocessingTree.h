#ifndef CREATEPREDPROCESSINGTREE
#define CREATEPREDPROCESSINGTREE

#include "../../General/programTree/tree.h"

/// @brief The function creates a tree from the tokens and writes it to a dot file to make graphical dump.
/// @param tokens the array of tokens from the lexical analysis
/// @param dotFile the file for saving the tree in .dot format
/// @param directory the directory where the dot file will be saved
/// @return Created tree of program.

node_t* createPredprocessingTree(node_t* tokens, const char* dotFile, const char* directory);

#endif // CREATEPREDPROCESSINGTREE