//
// Created by yaser on 1/23/2020.
//
#ifndef SPR_CLASS_H
#define SPR_CLASS_H

#include "common.h"
#include "utils.h"
#include "header_obj.h"
#include "obj_string.h"
#include "obj_fn.h"

typedef enum {
    MT_NONE,//空方法
    MT_PRIMITIVE,//原生方法
    MT_SCRIPT,//脚本方法
    MT_FN_CALL//用于函数重载
} MethodType;

//valueType transfer to value
#define VT_TO_VALUE(vt) ((Value) {vt, {0}})


#define BOOL_TO_VALUE(boolean) (boolean ? VT_TO_VALUE(VT_TRUE) : VT_TO_VALUE(VT_FALSE))
#define VALUE_TO_BOOL(value) ((value).type == VT_TRUE ? true : false)

#define NUM_TO_VALUE(num) ((Value){VT_NUM, {num}})
#define VALUE_TO_NUM(value) value.num

#define OBJ_TO_VALUE(objPtr) ({ \
   Value value; \
   value.type = VT_OBJ; \
   value.objHeader = (ObjHeader*)(objPtr); \
   value; \
})


#define VALUE_TO_OBJ(value) (value.objHeader)
#define VALUE_TO_OBJSTR(value) ((ObjString*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJFN(value) ((ObjFn*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJRANGE(value) ((ObjRange*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJINSTANCE(value) ((ObjInstance*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJLIST(value) ((ObjList*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJMAP(value) ((ObjMap*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJCLOSURE(value) ((ObjClosure*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJTHREAD(value) ((ObjThread*)VALUE_TO_OBJ(value))
#define VALUE_TO_OBJMODULE(value) ((ObjModule*)VALUE_TO_OBJ(value))
#define VALUE_TO_CLASS(value) ((Class*)VALUE_TO_OBJ(value))

#define VALUE_IS_UNDEFINED(value) ((value).type == VT_UNDEFINED)
#define VALUE_IS_NULL(value) ((value).type == VT_NULL)
#define VALUE_IS_TRUE(value) ((value).type == VT_TRUE)
#define VALUE_IS_FALSE(value) ((value).type == VT_FALSE)
#define VALUE_IS_NUM(value) ((value).type == VT_NUM)
#define VALUE_IS_OBJ(value) ((value).type == VT_OBJ)
#define VALUE_IS_CERTAIN_OBJ(value, objType) (VALUE_IS_OBJ(value) && VALUE_TO_OBJ(value)->type == objType)
#define VALUE_IS_OBJSTR(value) (VALUE_IS_CERTAIN_OBJ(value, OT_STRING))
#define VALUE_IS_OBJINSTANCE(value) (VALUE_IS_CERTAIN_OBJ(value, OT_INSTANCE))
#define VALUE_IS_OBJCLOSURE(value) (VALUE_IS_CERTAIN_OBJ(value, OT_CLOSURE))
#define VALUE_IS_OBJRANGE(value) (VALUE_IS_CERTAIN_OBJ(value, OT_RANGE))
#define VALUE_IS_CLASS(value) (VALUE_IS_CERTAIN_OBJ(value, OT_CLASS))
#define VALUE_IS_0(value) (VALUE_IS_NUM(value) && (value).num == 0)


//primitive method pointer
typedef bool (*Primitive)(VM *vm, Value *args);

typedef struct {
    MethodType type;
    union {
        //valid when type is MT_PRIMITIVE
        //point to the c implementation which associated with the script method
        Primitive primFn;
        //valid when type is MT_SCRIPT
        ObjClosure *obj;
    };
} Method;
DECLARE_BUFFER_TYPE(Method);

struct class {
    ObjHeader *objHeader;
    struct class *superClass;
    uint32_t fieldNum;
    MethodBuffer methods;
    ObjString *name;
};
typedef union {
    uint64_t bits64;
    uint32_t bits32[2];
    double num;
} Bits64;

bool valueIsEqual(Value a, Value b);

#define CAPACITY_GROW_FACTOR 4
#define MIN_CAPACITY 64
#endif //SPR_CLASS_H
