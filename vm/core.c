//
// Created by wanyu on 2019/10/29.
//
#include "core.h"
#include <string.h>
#include <sys/stat.h>
#include "utils.h"
#include "vm.h"

char *rootDir = null;

//read the source code from file
char *readFile(const char *path) {
    FILE *file = fopen(path, "r");//open the file
    if (isNull(file)) {
        IO_ERROR("can't open the file \"%s\".\n", path);
    }
    struct stat fileStat;
    stat(path, &fileStat);//get the file attribute
    size_t fileSize = fileStat.st_size;
    char *fileContent = (char *) malloc(fileSize + 1);
    if (isNull(fileContent)) {
        MEM_ERROR("can't allocate memory for reading file \"$s\".\n", path);
    }
    size_t numRead = fread(fileContent, sizeof(char), fileSize, file);//read data from file to fileContent buffer
    if (numRead < fileSize) {
        IO_ERROR("can't read file \"%s\".\n", path);
    }
    fileContent[fileSize] = '\0';//set the last as 0
    fclose(file);//close file
    return fileContent;
}
