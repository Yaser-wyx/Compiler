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
        uint32_t pathLen = lastSlash - filePath + 1;
        char *root = (char *) malloc(pathLen + 1);
        memcpy(root, filePath, pathLen);
        root[pathLen] = '\0';
        rootDir = root;
    }

    VM *vm = newVM();
    const char *sourceCode = readFile(filePath);
    struct parser parser;
    initParser(vm, &parser, filePath, sourceCode);

#include "token.list"

    while (parser.curToken.type != TOKEN_EOF) {
        getNextToken(&parser);
        //print token
        printf("%dL-tokenArray[%d]: %s [", parser.curToken.lineNo,
               parser.curToken.type, tokenArray[parser.curToken.type]);
        uint32_t idx = 0;
        while (idx < parser.curToken.length) {
            //print the source code of token
            printf("%c", *(parser.curToken.start + idx++));
        }
        printf("]\n");
    }
}

int main(int argc, const char **argv) {
    if (argc == 1) { ;//todo command line
    } else {
        runFile(argv[1]);
    }
    return 0;
}