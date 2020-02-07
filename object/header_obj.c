//
// Created by yaser on 1/23/2020.
//
#include "header_obj.h"
#include "class.h"
#include "vm.h"

DEFINE_BUFFER_METHOD(Value)

//初始化对象头部
void initObjHeader(VM *vm, ObjHeader *objHeader, ObjType objType, Class *class) {
    objHeader->type = objType;//设置对象类型
    objHeader->isReachable = false;//设置该对象是否为可到达状态，用于GC
    objHeader->class = class;
    //将新的对象头部插入到虚拟机所有对象列表的首部
    objHeader->next = vm->allObjects;
    vm->allObjects = objHeader;
}
