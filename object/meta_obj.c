//
// Created by yaser on 1/23/2020.
//
#include "obj_fn.h"
#include "class.h"
#include "vm.h"
#include <string.h>
#include "meta_obj.h"

//创建新的模块对象
ObjModule *newObjModule(VM *vm, const char *moduleName) {
    ObjModule *objModule = ALLOCATE(vm, ObjModule);//为objModule在虚拟机中分配内存
    if (objModule == null) {
        //内存分配失败
        MEM_ERROR("allocate memory for module object failed!");
    } else {
        //为新的对象模块设置对象头，因为模块没有类模板，所以class传null
        initObjHeader(vm, &objModule->objHeader, OT_MODULE, null);
        StringBufferInit(&objModule->moduleVarName);
        ValueBufferInit(&objModule->moduleVarValue);
        objModule->name = null;//默认为核心模块，核心模块名字为null
        if (moduleName != null) {
            //如果是用户自定义模块，则重命名
            objModule->name = newObjString(vm, moduleName, strlen(moduleName));
        }
    }
    //返回创建完成的新模块
    return objModule;
}

//创建类模板的对象实例
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