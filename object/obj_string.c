//
// Created by yaser on 1/23/2020.
//
#include "obj_string.h"
#include <string.h>
#include "vm.h"
#include "utils.h"
#include "common.h"
#include <stdlib.h>

//used to hash string to hashCode, FNV algorithm.
uint32_t hashString(char *str, uint32_t length) {
    uint32_t hashCode = 2166136261, idx = 0;
    while (idx < length) {
        hashCode ^= str[idx];
        hashCode *= 16777619;
        idx++;
    }
    return hashCode;
}

void hashObjString(ObjString *objString) {
    objString->hashCode = hashString(objString->value.start, objString->value.length);
}

//create string object for str
ObjString *newObjString(VM *vm, const char *str, uint32_t length) {
    ASSERT(length == 0 || str != null, "str length don't match str");
    ObjString *objString = ALLOCATE_EXTRA(vm, ObjString, length + 1);
    if (objString != null) {
        initObjHeader(vm, &objString->objHeader, OT_STRING, vm->stringClass);
        objString->value.length = length;
        if (length > 0) {
            memcpy(objString->value.start, str, length);
        }
        objString->value.start[length] = '\0';
        hashObjString(objString);
    }
    return objString;
}
