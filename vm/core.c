//
// Created by wanyu on 2019/10/29.
//
#include "core.h"
#include <string.h>
#include <sys/stat.h>
#include "utils.h"
#include "vm.h"
#include "meta_obj.h"

char *rootDir = null;
#define CORE_MODULE VT_TO_VALUE(VT_NULL)


//从文件中读取源码
char *readFile(const char *path) {
    FILE *file = fopen(path, "r");//打开文件
    if (isNull(file)) {
        IO_ERROR("can't open the file \"%s\".\n", path);
    }
    struct stat fileStat;
    stat(path, &fileStat);//获取文件属性
    size_t fileSize = fileStat.st_size;
    char *fileContent = (char *) malloc(fileSize + 1);//为文件缓冲区分配内存，其中+1为了最后置0
    if (isNull(fileContent)) {
        MEM_ERROR("can't allocate memory for reading file \"$s\".\n", path);
    }
    size_t numRead = fread(fileContent, sizeof(char), fileSize, file);//从文件中读取数据到文件缓冲区中，并返回读入的数据量
    if (numRead < fileSize) {
        IO_ERROR("can't read file \"%s\".\n", path);
    }
    fileContent[fileSize] = '\0';//将缓冲区中的最后一位置0
    fclose(file);
    return fileContent;
}

VMResult executeModule(VM *vm, Value moduleName, const char *moduleCode) {
    //todo 站桩函数，目前为空
    return VM_RESULT_ERROR;
}


void buildCore(VM *vm) {
    //编译核心模块
    ObjModule *coreModule = newObjModule(vm, null);
    mapSet(vm, vm->allModules, CORE_MODULE, OBJ_TO_VALUE(coreModule));
}
