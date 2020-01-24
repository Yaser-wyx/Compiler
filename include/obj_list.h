//
// Created by yaser on 1/24/2020.
//

#ifndef TEMP_OBJ_LIST_H
#define TEMP_OBJ_LIST_H

#include "class.h"
#include "vm.h"

typedef struct {
    ObjHeader objHeader;
    ValueBuffer elements;//element in list
} ObjList;//list object


ObjList* newObjList(VM* vm, uint32_t elementNum);
Value removeElement(VM* vm, ObjList* objList, uint32_t index);
void insertElement(VM* vm, ObjList* objList, uint32_t index, Value value);
#endif //TEMP_OBJ_LIST_H
