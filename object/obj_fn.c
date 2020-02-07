//
// Created by yaser on 1/24/2020.
//

#include "meta_obj.h"
#include "class.h"
#include "vm.h"

//create a function object
ObjFn *newObjFn(VM *vm, ObjModule *objModule, uint32_t slotNum) {
    //slotNum means the stack space for function
    ObjFn *objFn = ALLOCATE(vm, ObjFn);
    initObjHeader(vm, &objFn->objHeader, OT_FUNCTION, vm->fnClass);
    ByteBufferInit(&objFn->instrStream);
    ValueBufferInit(&objFn->constants);
    objFn->module = objModule;
    objFn->argNum = objFn->upvalueNum = 0;
    objFn->maxStackSlotUsedNum = slotNum;

#ifdef DEBUG
    objFn->debug = ALLOCATE(vm,FnDebug);
    objFn->debug->fnName = null;
    IntBufferInit(&objFn->debug->lineNo);
#endif
    return objFn;
}

//将方法对象封装为closure对象
ObjClosure *newObjClosure(VM *vm, ObjFn *objFn) {
    ObjClosure *objClosure = ALLOCATE_EXTRA(vm, ObjClosure, sizeof(ObjUpvalue *) * objFn->upvalueNum);
    initObjHeader(vm, &objClosure->objHeader, OT_CLOSURE, vm->fnClass);
    objClosure->fn = objFn;
    //使用null值来预先填充upvalue数组，避免提前触发GC
    uint32_t idx = 0;
    while (idx < objFn->upvalueNum) {
        objClosure->upvalues[idx++] = null;
    }
    return objClosure;
}

//create upvalue object
ObjUpvalue *newObjUpvalue(VM *vm, Value *localVarPtr) {
    //localVarPtr point to the local variable address which in stack
    ObjUpvalue *objUpvalue = ALLOCATE(vm, ObjUpvalue);
    initObjHeader(vm, &objUpvalue->objHeader, OT_UPVALUE, null);
    objUpvalue->localVarPtr = localVarPtr;
    objUpvalue->closedUpvalue = VT_TO_VALUE(VT_NULL);
    objUpvalue->next = null;
    return objUpvalue;
}