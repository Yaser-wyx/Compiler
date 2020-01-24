//
// Created by yaser on 1/23/2020.
//
#include "obj_fn.h"
#include "class.h"
#include "vm.h"
#include <string.h>
#include "meta_obj.h"

//new module
ObjModule *newObjModule(VM *vm, const char *moduleName) {
    ObjModule *objModule = ALLOCATE(vm, ObjModule);//allocate memory for ObjModule
    if (objModule == null) {
        MEM_ERROR("allocate memory for module object failed!");
    } else {
        //ObjModule not belong to any class.
        initObjHeader(vm, &objModule->objHeader, OT_MODULE, null);
        StringBufferInit(&objModule->moduleVarName);
        ValueBufferInit(&objModule->moduleVarValue);
        objModule->name = null;//core module named null.
        if (moduleName != null) {
            //if module is user-defined
            objModule->name = newObjString(vm, moduleName, strlen(moduleName));
        }
    }
    return objModule;
}

//create class instance
ObjInstance *newObjInstance(VM *vm, Class *class) {
    ObjInstance *objInstance = ALLOCATE_EXTRA(vm, ObjInstance, sizeof(Value) * class->fieldNum);
    initObjHeader(vm, &objInstance->objHeader, OT_INSTANCE, class);
    uint32_t idx = 0;
    while (idx < class->fieldNum) {
        objInstance->fields[idx] = VT_TO_VALUE(VT_NULL);
        idx++;
    }
    return objInstance;
}