//
// Created by yaser on 1/24/2020.
//
#include "class.h"
#include "common.h"
#include "string.h"
#include "obj_range.h"
#include "core.h"
#include "vm.h"

DEFINE_BUFFER_METHOD(Method)

bool valueIsEqual(Value a, Value b) {
    //determine if the values are equal
    if (a.type != b.type) {
        //if type is differ, then return false directly
        return false;
    }
    if (a.type == VT_NUM && b.type == VT_NUM) {
        //type is number
        return a.num == b.num;
    }
    if (a.objHeader == b.objHeader) {
        //the same obj
        return true;
    }
    if (a.objHeader->type != b.objHeader->type) {
        //obj type is differ
        return false;
    }
    //handle the same type object

    if (a.objHeader->type == OT_RANGE) {
        //range obj
        ObjRange *objRangeA = VALUE_TO_OBJRANGE(a);
        ObjRange *objRangeB = VALUE_TO_OBJRANGE(b);
        return objRangeA->from == objRangeB->from && objRangeA->to == objRangeB->to;
    }
    if (a.objHeader->type == OT_STRING) {
        //string obj
        ObjString *objStringA = VALUE_TO_OBJSTR(a);
        ObjString *objStringB = VALUE_TO_OBJSTR(a);
        return objStringA->value.length == objStringB->value.length &&
               memcmp(objStringA->value.start, objStringB->value.start, objStringA->value.length);
    }

    return false;
}

//新建一个裸类，可认为是所有类的父类
Class *newRawClass(VM *vm, const char *name, uint32_t fieldNum) {
    Class *rawClass = ALLOCATE(vm, Class);//分配内存
    initObjHeader(vm, rawClass->objHeader, OT_CLASS, null);
    rawClass->name = newObjString(vm, name, strlen(name));
    rawClass->fieldNum = fieldNum;
    rawClass->superClass = null;
    MethodBufferInit(&rawClass->methods);
    return rawClass;
}

//通过形参obj来判断该对象的归属类是什么
inline Class *getClassOfObj(VM *vm, Value obj) {
    switch (obj.type) {
        case VT_NULL:
            return vm->nullClass;
        case VT_FALSE:
        case VT_TRUE:
            return vm->boolClass;
        case VT_NUM:
            return vm->numClass;
        case VT_OBJ:
            return VALUE_TO_OBJ(obj)->class;
        default:
            NOT_REACHED()
    }
    return null;
}