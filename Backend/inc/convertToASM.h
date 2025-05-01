#ifndef CONVERTTOASM_H
#define CONVERTTOASM_H

#include "programTree/tree.h"

/// @brief Converts the program tree to assembly code and writes it to a file.
/// @param node the root of the program tree
/// @param nameTable array which contains the names of the variables
/// @param asmFile the file name where the assembly code will be written

void writeASMfile(node_t* node, nameTable_t* nameTable, const char* asmFile);
void writeNASM64(node_t* node, const char* asmFile);

#endif // CONVERTTOASM_H