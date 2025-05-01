#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "programTree/tree.h"
#include "treeTransfer/treeTransfer.h"
#include "graphDump/graphDump.h"
#include "convertToASM.h"
#include "constants.h" // Include the constants header

void delTable(nameTable_t* nameTable);

int main()
{
    nameTable_t* nameTable = (nameTable_t*)calloc(c_nameTableSize, sizeof(nameTable_t)); // Use the constant here
    assert(nameTable != nullptr);
    if (nameTable == nullptr)
    {
        printf("Error: Allocate memory fail!\n");
    }
    nameTable_t* startOfNameTable = nameTable;
    node_t* progTree = pullTree(&nameTable, "../progTree");
    assert(progTree != nullptr);
    writeDotFile(progTree, "../dot_files/backendDotFile.dot");
    writePngFile("../dot_files/backendDotFile.dot", "../png_files", "white");


    //writeASMfile(progTree, nameTable, "../program.ASM");
    writeNASM64(progTree, "../program.ASM");

    delTree(progTree);
    progTree = nullptr;
    
    delTable(startOfNameTable);
    nameTable = nullptr;
    startOfNameTable = nullptr;
    return 0;
}

void delTable(nameTable_t* nameTable)
{
    assert(nameTable != nullptr);
    size_t i = 0;
    while (nameTable[i].str != nullptr)
    {
        free(nameTable[i].str);
        nameTable[i++].str = nullptr;
    }
    free(nameTable);
}