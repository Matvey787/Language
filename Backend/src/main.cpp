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

int main()
{
    node_t* progTree = pullTree("../progTree");
    assert(progTree != nullptr);

    writeDotFile(progTree, "../dot_files/backendDotFile.dot");
    writePngFile("../dot_files/backendDotFile.dot", "../png_files", "white");
    
    writeNASM64(progTree, "../program.ASM");

    delTree(progTree);
    progTree = nullptr;
    return 0;
}
