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
//如果执行结果为success，则将字节码输出到文件进行缓存，避免重复编译
struct vm {
    //内置class类型
    Class *classOfClass;
    Class *objectClass;//所有类的父类
    Class *stringClass;
    Class *mapClass;
    Class *rangeClass;
    Class *listClass;
    Class *nullClass;
    Class *boolClass;
    Class *numClass;
    Class *fnClass;
    Class *threadClass;

    ObjHeader *allObjects;//所有对象列表
    uint32_t allocatedBytes;//已分配内存大小
    SymbolTable allMethodNames;//存储所有方法名的数组，该数组中每一个方法名对应的方法体在指定class中methods的相同索引位置上，是一对一映射关系
    ObjMap *allModules;//模块hash表
    ObjThread *curThread;//当前线程
    Parser *curParser;//当前词法分析器

};

void initVM(VM *);

VM *newVM(void);

#endif //_VM_VM_H
