//
// Created by wanyu on 2019/10/29.
//
#include "core.h"
#include <string.h>
#include <sys/stat.h>
#include "utils.h"
#include "vm.h"

char *rootDir = NULL;//根目录

char *readFile(const char *path) {
    FILE *file = fopen(path, "r");//打开指定路径的文件
    if (file == NULL) {
        IO_ERROR("Could't open file \"%s\".\n", path);
    }
    struct stat fileStat;//创建文件状态
    stat(path, &fileStat);//读取该路径文件的信息
    size_t fileSize = fileStat.st_size;//获取文件大小
    char *fileContent = (char *) malloc(fileSize + 1);//根据文件大小来分配文件容器的内存空间
    if (fileContent == NULL) {
        MEM_ERROR("Could't allocate memory for reading file \"%s\".\n", path);
    }
    size_t numRead = fread(fileContent, sizeof(char), fileSize, file);//从文件中读取数据到容器中
    if ((void *) numRead == NULL) {
        IO_ERROR("Could't read file  \"%s\".\n", path);
    }
    fileContent[fileSize] = '\0';//将文件容器的最后一个字节置0
    fclose(file);//关闭文件
    return fileContent;
}
