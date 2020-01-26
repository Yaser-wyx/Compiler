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
        //the code file maybe not exist in the current path
        uint32_t pathLen = lastSlash - filePath + 1;//cal the length of path
        char *root = (char *) malloc(pathLen + 1);//allocate memory for root path string
        memcpy(root, filePath, pathLen);
        root[pathLen] = '\0';
        rootDir = root;
    }
    //create a VM
    VM *vm = newVM();
    const char *sourceCode = readFile(filePath);
    executeModule(vm, OBJ_TO_VALUE(newObjString(vm, filePath, strlen(filePath))), sourceCode);
}

int main(int argc, const char **argv) {
    if (argc == 1) { ;//todo command line
    } else {
        runFile(argv[1]);
    }
    return 0;
}