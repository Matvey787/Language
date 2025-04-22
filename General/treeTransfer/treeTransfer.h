#ifndef TREETRANSFER_H
#define TREETRANSFER_H

#include "../programTree/tree.h"

/// @brief Pushes the tree to a file for transfer
/// @param node the pointer to the root of the tree
/// @param transferFileName the name of the file to push the tree to

void pushTree(node_t* node, const char* transferFileName);

/// @brief Pulls the program tree from a file for transfer
/// @param nameTable the name table to save vars for the prog tree
/// @param transferFileName the name of the file to pull the tree from
/// @return the pointer to the root of the prog tree

node_t* pullTree(nameTable_t** nameTable, const char* transferFileName);

#endif // TREETRANSFER_H