//
// Created by yaser on 1/23/2020.
//

#ifndef SPR_META_OBJ_H
#define SPR_META_OBJ_H

#include "obj_string.h"

typedef struct {
    ObjHeader objHeader;
    SymbolTable moduleVarName; //module name
    ValueBuffer moduleVarValue;//module value
    ObjString *name;
} ObjModule;//the module Object

typedef struct {
    ObjHeader objHeader;
    //concrete fields
    Value fields[0];
}ObjInstance;//the instance of Obj

ObjModule* newObjModule(VM* vm, const char* modName);
ObjInstance* newObjInstance(VM* vm, Class* class);
#endif //SPR_META_OBJ_H
