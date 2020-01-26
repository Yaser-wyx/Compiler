//
// Created by wanyu on 2019/10/29.
//

#ifndef _VM_VM_H
#define _VM_VM_H

#include "common.h"
#include "class.h"
#include "parser.h"
#include "obj_map.h"

typedef enum vmResult {
    VM_RESULT_SUCCESS,
    VM_RESULT_ERROR
} VMResult;//虚拟机执行结果
//如果执行无误，则将字节码输出到文件进行缓存，避免重复编译
struct vm {
    Class *classOfClass;
    Class *objectClass;
    Class *stringClass;
    Class *mapClass;
    Class *rangeClass;
    Class *listClass;
    Class *nullClass;
    Class *boolClass;
    Class *numClass;
    Class *fnClass;
    Class *threadClass;
    ObjHeader *allObjects;//obj list
    uint32_t allocatedBytes;
    SymbolTable allMethodNames;
    ObjMap *allModules;
    ObjThread *curThread;
    Parser *curParser;

};

void initVM(VM *);

VM *newVM(void);

#endif //_VM_VM_H
