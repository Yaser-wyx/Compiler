//
// Created by yaser on 1/23/2020.
//
#include "header_obj.h"
#include "class.h"
#include "vm.h"

DEFINE_BUFFER_METHOD(Value)

void initObjHeader(VM *vm, ObjHeader *objHeader, ObjType objType, Class *class) {
    objHeader->type = objType;
    objHeader->isReachable = false;
    objHeader->class = class;
    objHeader->next = vm->allObjects;
    vm->allObjects = objHeader;
}
