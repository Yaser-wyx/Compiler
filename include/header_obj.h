//
// Created by yaser on 1/23/2020.
//

#ifndef SPR_HEADER_OBJ_H
#define SPR_HEADER_OBJ_H

#include "utils.h"

typedef enum {
    OT_CLASS,
    OT_LIST,
    OT_MAP,
    OT_MODULE,
    OT_RANGE,
    OT_STRING,
    OT_UPVALUE,
    OT_FUNCTION,
    OT_CLOSURE,
    OT_INSTANCE,
    OT_THREAD
} ObjType;

typedef struct objHeader {
    ObjType type;//对象类型
    bool isReachable;//该对象是否为可达状态，用于GC
    Class *class;//归属类，用于对象调用类的内部方法
    struct objHeader *next;//指向下一个对象的对象头
} ObjHeader;

typedef enum {
    VT_UNDEFINED,
    VT_NULL,
    VT_FALSE,
    VT_TRUE,
    VT_NUM,
    VT_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        double num;
        ObjHeader *objHeader;
    };
} Value;//统一的值结构类型，可以存数值，也可以存对象

DECLARE_BUFFER_TYPE(Value);

void initObjHeader(VM *vm, ObjHeader *objHeader, ObjType objType, Class *class);

#endif //SPR_HEADER_OBJ_H
