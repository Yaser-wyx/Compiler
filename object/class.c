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