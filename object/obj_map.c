//
// Created by yaser on 1/24/2020.
//
#include "obj_map.h"
#include "class.h"
#include "vm.h"
#include "obj_string.h"
#include "obj_range.h"

ObjMap *newObjMap(VM *vm) {
    ObjMap *objMap = ALLOCATE(vm, ObjMap);
    initObjHeader(vm, &objMap->objHeader, OT_MAP, vm->mapClass);
    objMap->capacity = objMap->count = 0;
    objMap->entries = null;
    return objMap;
}

//calculate the num hashCode
static uint32_t hashNum(double num) {
    Bits64 bits64;
    bits64.num = num;
    return bits64.bits32[0] ^ bits64.bits32[1];
}

//calculate the obj hashcode
static uint32_t hashObj(ObjHeader *objHeader) {
    switch (objHeader->type) {
        case OT_CLASS: {
            return hashString(((Class *) objHeader)->name->value.start, ((Class *) objHeader)->name->value.length);
        }
        case OT_RANGE: {
            ObjRange *objRange = (ObjRange *) objHeader;
            return hashNum(objRange->from) ^ hashNum(objRange->to);
        }
        case OT_STRING: {
            return ((ObjString *) objHeader)->hashCode;
        }
        default:
            RUN_ERROR("the hashable are string, range and class.");
    }
    return 0;
}

static uint32_t hashValue(Value value) {
    switch (value.type) {
        case VT_NUM:
            return hashNum(value.num);
        case VT_FALSE:
            return 0;
        case VT_NULL:
            return 1;
        case VT_TRUE:
            return 2;
        case VT_OBJ:
            return hashObj(value.objHeader);
        default:
            RUN_ERROR("unsupported type to hash!");
    }
    return 0;
}

static bool addEntry(Entry *entries, uint32_t capacity, Value key, Value value) {
    //calculate slot index
    uint32_t index = hashValue(value) % capacity;

    while (true) {
        if (entries[index].key.type == VT_UNDEFINED) {
            //empty slot
            entries[index].key = key;
            entries[index].value = value;
            return true;
        } else if (valueIsEqual(key, entries[index].key)) {
            entries[index].value = value;
            return false;
        }
        //relocate slot index
        index = (index + 1) % capacity;
    }
}