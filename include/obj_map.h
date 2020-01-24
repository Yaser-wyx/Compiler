//
// Created by yaser on 1/24/2020.
//

#ifndef TEMP_OBJ_MAP_H
#define TEMP_OBJ_MAP_H

#include "header_obj.h"

#define MAP_LOAD_PERCENT 0.8

typedef struct {
    Value key;
    Value value;
} Entry; // key->value
typedef struct {
    ObjHeader objHeader;
    uint32_t capacity; //entry capacity
    uint32_t count; //the num of entry
    Entry *entries;//entry array
} ObjMap;

ObjMap *newObjMap(VM *vm);

void mapSet(VM *vm, ObjMap *objMap, Value key, Value value);

Value mapGet(ObjMap *objMap, Value key);

void clearMap(VM *vm, ObjMap *objMap);

Value removeKey(VM *vm, ObjMap *objMap, Value key);

#endif //TEMP_OBJ_MAP_H
