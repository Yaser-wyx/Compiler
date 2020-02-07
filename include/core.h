//
// Created by wanyu on 2019/10/29.
//

#ifndef COMPILER_CORE_H
#define COMPILER_CORE_H

#include "vm.h"

extern char *rootDir;

char *readFile(const char *path);

VMResult executeModule(VM *vm, Value moduleName, const char *moduleCode);

void buildCore(VM *vm);

int getIndexFromSymbolTable(SymbolTable* table, const char* symbol, uint32_t length);
int addSymbol(VM* vm, SymbolTable* table, const char* symbol, uint32_t length);
void bindMethod(VM* vm, Class* class, uint32_t index, Method method);
void bindSuperClass(VM* vm, Class* subClass, Class* superClass);

#endif //COMPLIER_CORE_H
