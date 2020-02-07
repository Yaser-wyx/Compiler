//
// Created by yaser on 1/23/2020.
//

#ifndef SPR_META_OBJ_H
#define SPR_META_OBJ_H

#include "obj_string.h"

typedef struct {
    ObjHeader objHeader;//模块的对象头
    SymbolTable moduleVarName; //模块中的模块变量名
    ValueBuffer moduleVarValue;//模块中的模块变量值
    ObjString *name;
} ObjModule;//模块对象

typedef struct {
    ObjHeader objHeader;//实例对象头

    Value fields[0];//每一个对象的内部属性，使用柔性数组来表达
}ObjInstance;//对象实例

ObjModule* newObjModule(VM* vm, const char* modName);
ObjInstance* newObjInstance(VM* vm, Class* class);
#endif //SPR_META_OBJ_H
