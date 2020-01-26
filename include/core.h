//
// Created by wanyu on 2019/10/29.
//

#ifndef COMPLIER_CORE_H
#define COMPLIER_CORE_H

#include "vm.h"

extern char *rootDir;

char *readFile(const char *path);

VMResult executeModule(VM *vm, Value moduleName, const char *moduleCode);

void buildCore(VM *vm);

#endif //COMPLIER_CORE_H
