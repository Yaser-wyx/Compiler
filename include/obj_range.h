//
// Created by yaser on 1/24/2020.
//

#ifndef TEMP_OBJ_RANGE_H
#define TEMP_OBJ_RANGE_H

#include "class.h"

typedef struct {
    ObjHeader objHeader;
    int from;
    int to;
} ObjRange;

ObjRange *newObjRange(VM *vm, int from, int to);

#endif //TEMP_OBJ_RANGE_H
