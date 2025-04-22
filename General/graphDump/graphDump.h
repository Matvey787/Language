#ifndef GRAPHDUMP
#define GRAPHDUMP

#include "../../General/programTree/tree.h"

const char* getColor(types type);
const char* convertTypeToStr(types type);

/**
 * @brief This function write info about tree of our program to a dot file. In the future, this 
 * file will be used as the basis of graph dump.
 * @param node A pointer to a tree which should be written to a dot file.
 * @param file_name The path to the file to be write.
 */

void writeDotFile(node_t* node, const char* fileName);

/**
 * @brief This function create png file from dot file. If it doesn't work, try to download graphviz to your pc.
 *
 * @param dotFile The path to the dot file to be convert.
 * @param directory The path to the directory where the png file will be created.
 * @param file_name The path to the file to be write.
 * @param backGrColor The color of the background of the png file.
 * 
 * @return How many times this function was called (to correctly creating file names in the project)
 */

int writePngFile(const char* dotFile, const char* directory, const char* backGrColor);

#endif