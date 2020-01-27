//
// Created by wanyu on 2019/10/29.
//
#include "vm.h"
#include <stdlib.h>
#include "utils.h"
#include "core.h"

void initVM(VM *vm) {
    vm->allocatedBytes = 0;
    vm->curParser = null;
    vm->allObjects = null;
    StringBufferInit(&vm->allMethodNames);
    vm->allModules = newObjMap(vm);
    vm->curParser = null;
}

VM *newVM() {
    //为虚拟机分配内存空间
    VM *vm = (VM *) malloc(sizeof(VM));
    if (isNull(vm)) {
        //内存分配失败，报告内存错误
        MEM_ERROR("allocate VM failed!");
    }
    //初始化虚拟机并编译核心模块
    initVM(vm);
    buildCore(vm);
    return vm;
}