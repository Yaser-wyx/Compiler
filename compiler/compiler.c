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
//作用域结构
struct compileUnit {
    ObjFn *fn;//所编译的函数
    localVar localVars[MAX_LOCAL_VAR_NUM];//作用域中的局部变量名字
    uint32_t localVarNum;//已分配的局部变量个数，同时记录下一个可用与存储局部变量的索引
    Upvalue upvalues[MAX_UPVALUE_NUM];//本层函数引用的upvalue
    int scopeDepth;//当前编译的代码所处的作用域，-1表示模块作用域，0表示没有嵌套，即最外层，1及以上表示对应的嵌套层
    uint32_t stackSlotNum;//统计编译单元内对栈的影响
    Loop *curLoop;//当前正在编译的循环层
    ClassBookKeep *enclosingClassBK;//正在编译的类的编译信息
    struct compileUnit *enclosingUnit;//包含此编译单元的编译单元
    Parser *curParser;//当前parser
};

int defineModuleVar(VM *vm, ObjModule *objModule, const char *name, uint32_t length, Value value) {
    //注：此处的value不是变量的值，只是用于编译阶段占位使用，value取值为null或行号（用于引用变量，却未定义的情况）,变量具体的值会在虚拟机运行阶段确定
    if (length > MAX_ID_LEN) {
        //标识符长度太长
        char id[MAX_ID_LEN] = {'\0'};
        memcpy(id, name, length);

        if (vm->curParser != null) {
            //编译阶段
            COMPILE_ERROR(vm->curParser, "length of identifier \"%s\" should be no more than %d", id, MAX_ID_LEN);
        } else {
            // 编译源码前调用,比如加载核心模块时会调用本函数
            MEM_ERROR("length of identifier \"%s\" should be no more than %d", id, MAX_ID_LEN);
        }
    }
    //从模块名符号表中查找该名字是否存在
    int symbolIndex = getIndexFromSymbolTable(&objModule->moduleVarName, name, length);
    if (symbolIndex == -1) {
        //如果不存在则进行添加
        symbolIndex = addSymbol(vm, &objModule->moduleVarName, name, length);//添加变量名，并获得添加标识符后的索引
        ValueBufferAdd(vm, &objModule->moduleVarValue, value);//添加变量值
    } else if (VALUE_IS_NUM(objModule->moduleVarValue.datas[symbolIndex])) {
        //如果是数字类型，则表示该变量在之前未定义，现在进行定义
        objModule->moduleVarValue.datas[symbolIndex] = value;
    } else {
        symbolIndex = -1;//如果重复定义了，就返回-1
    }
    return symbolIndex;
}

//编译模块
ObjFn* compileModule(VM* vm, ObjModule* objModule, const char* moduleCode){
    ;
    //TODO 站桩用，待填充
}