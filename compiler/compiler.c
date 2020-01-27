//
// Created by yaser on 1/27/2020.
//

#include "compiler.h"
#include "parser.h"
#include "core.h"
#include "string.h"

#if DEBUG
#include "debug.h"
#endif

struct compileUnit {
    ObjFn *fn;//编译的函数
    localVar localVars[MAX_LOCAL_VAR_NUM];//作用域中的局部变量
    uint32_t localVarNum;//已分配的局部变量个数，当前未使用的局部变量指针
    Upvalue upvalues[MAX_UPVALUE_NUM];//本层函数引用的upvalue
    int scopeDepth;//当前编译的代码所处的作用域
    uint32_t stackSlotNum;//统计编译单元内对栈的影响
    Loop *curLoop;//当前正在编译的循环层
    ClassBookKeep *enclosingClassBK;//正在编译的类的编译信息
    struct compileUnit *enclosingUnit;//包含此编译单元的编译单元
    Parser *curParser;//当前parser
};