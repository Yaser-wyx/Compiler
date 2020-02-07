//
// Created by yaser on 1/22/2020.
//
#include "cli.h"
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "vm.h"
#include "core.h"

static void runFile(const char *filePath) {
    const char *lastSlash = strrchr(filePath, '/');
    if (lastSlash != null) {
        //如果要读取的源码文件在其它路径下
        uint32_t pathLen = lastSlash - filePath + 1;//计算路径长度
        char *root = (char *) malloc(pathLen + 1);//为路径分配内存
        memcpy(root, filePath, pathLen);//将源码文件路径复制
        root[pathLen] = '\0';//最后一位置0
        rootDir = root;//设置虚拟机根路径
    }
    //构建虚拟机
    VM *vm = newVM();
    const char *sourceCode = readFile(filePath);//获取源码缓冲数据流
    //将源码视为模块并执行
    executeModule(vm, OBJ_TO_VALUE(newObjString(vm, filePath, strlen(filePath))), sourceCode);
}

int main(int argc, const char **argv) {
    if (argc == 1) { ;//todo command line
    } else {
        runFile(argv[1]);//将第二个参数，即文件路径传给runFile函数
    }
    return 0;
}