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
    ObjType type;
    bool isReachable;//for GC
    Class *class;//class for object
    struct objHeader *next;// pointer for
} ObjHeader;//used to record meta and GC

typedef enum {
    VT_UNDEFINED,
    VT_NULL,
    VT_FALSE,
    VT_TRUE,
    VT_NUM,
    VT_OBJ  //point to ObjHeader
} ValueType;

typedef struct {
    ValueType type;
    union {
        double num;
        ObjHeader *objHeader;
    };
} Value;//generic value struct

DECLARE_BUFFER_TYPE(Value);

void initObjHeader(VM *vm, ObjHeader *objHeader, ObjType objType, Class *class);
#endif //SPR_HEADER_OBJ_H
