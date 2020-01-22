//
// Created by wanyu on 2019/10/29.
//
#include "vm.h"
#include <stdlib.h>
#include "utils.h"

void initVM(VM *vm) {
    vm->allocatedBytes = 0;
    vm->curParser = null;
}

VM *newVM() {
    VM *vm = (VM *) malloc(sizeof(VM));
    if (isNull(vm)) {
        //if vm is null,report memory error
        MEM_ERROR("allocate VM failed!");
    }
    initVM(vm);
    return vm;
}