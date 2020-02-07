//
// Created by yaser on 1/24/2020.
//
#ifndef SPR_OBJ_FN_H
#define SPR_OBJ_FN_H

#include "utils.h"
#include "meta_obj.h"

typedef struct {
    char *fnName; //function name
    IntBuffer lineNo;//line number
} FnDebug;//debug struct

typedef struct {
    ObjHeader objHeader;
    ByteBuffer instrStream;//instruction stream for the compiled functions
    ValueBuffer constants;//constants table in function.

    ObjModule *module;//the module for function

    uint32_t maxStackSlotUsedNum;//max space for stack
    uint32_t upvalueNum;//the num of upvalues
    uint8_t argNum;//the num of arguments.
#if DEBUG
    FnDebug *debug
#endif
} ObjFn;//函数对象

typedef struct upvalue {
    ObjHeader objHeader;
    Value *localVarPtr;//localVarPtr point to the local value associate with upvalue

    Value closedUpvalue;//upvalue which has closed
    struct upvalue *next;//link the openUpValue list
} ObjUpvalue;//upvalue object

typedef struct {
    ObjHeader objHeader;
    ObjFn *fn;//the function which referenced by closure
    ObjUpvalue *upvalues[0];//used to save close upvalue

} ObjClosure;

typedef struct {
    uint8_t *ip;//point to the next instruction
    //在本frame中要执行的闭包函数
    ObjClosure *closure;
    //frame是共享therad.stack
    //此项用于指向本frame所在thread运行时栈的起始地址
    Value *stackStart;
} Frame;//invocation framework

#define INITIAL_FRAME_NUM 4

ObjUpvalue *newObjUpvalue(VM *vm, Value *localVarPtr);

ObjClosure *newObjClosure(VM *vm, ObjFn *objFn);

ObjFn *newObjFn(VM *vm, ObjModule *objModule, uint32_t maxStackSlotUsedNum);

#endif //SPR_OBJ_FN_H
