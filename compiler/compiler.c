//
// Created by yaser on 1/27/2020.
//

#include "compiler.h"
#include "parser.h"
#include "core.h"
#include "string.h"
#include "utils.h"

#define CORE_MODULE null
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

//定义操作码对栈的影响，从而可以使用操作码来读取opCodeSlotsUsed中的数据
#define OPCODE_SLOTS(opCode, effect) effect,
static const int opCodeSlotsUsed[] = {
#include "opcode.inc"
};
#undef OPCODE_SLOTS

//初始化CompileUnit
static void initCompileUnit(Parser *parser, CompileUnit *compileUnit, CompileUnit *enclosingUnit, bool isMethod) {
    parser->curCompileUnit = compileUnit;
    compileUnit->curParser = parser;
    compileUnit->enclosingClassBK = null;
    compileUnit->curLoop = null;
    compileUnit->enclosingUnit = enclosingUnit;

    if (enclosingUnit == null) {
        //如果外层没有单元，则表明当前属于模块作用域
        compileUnit->scopeDepth = -1;//模块作用域设为-1
        compileUnit->localVarNum = 0;//模块作用域内没有局部变量
    } else {
        //如果有外层单元，则当前作用域属于局部作用域
        if (isMethod) {
            //如果是类中的方法
            //则将当前编译单元的首个局部变量设置为"this"
            compileUnit->localVars[0].name = "this";
            compileUnit->localVars[0].length = 4;
        } else {
            //判断为普通函数
            //与方法统一格式
            compileUnit->localVars[0].name = null;
            compileUnit->localVars[0].length = 0;
        }
        //第0个局部变量的特殊性使其作用域为模块级别
        compileUnit->localVars[0].scopeDepth = -1;
        compileUnit->localVars->isUpvalue = false;
        compileUnit->localVarNum = 1; // localVars[0]被分配
        // 对于函数和方法来说,初始作用域是局部作用域
        // 0表示局部作用域的最外层
        compileUnit->scopeDepth = 0;
    }
    //局部变量保存在栈中，所以初始化的时候，栈中已使用的slot数量等于局部变量的个数
    compileUnit->stackSlotNum = compileUnit->localVarNum;
    compileUnit->fn = newObjFn(compileUnit->curParser->vm, compileUnit->curParser->curModule, compileUnit->localVarNum);
}

//想函数的指令流中写入一个字节的数据，并返回其索引
static int writeByte(CompileUnit *compileUnit, int byte) {
#if DEBUG
    IntBufferAdd(compileUnit->curParser->vm,&compileUnit->fn->debug->lineNo,compileUnit->curParser->preToken.lineNo);
#endif
    ByteBufferAdd(compileUnit->curParser->vm, &compileUnit->fn->instrStream, (uint8_t) byte);
    return compileUnit->fn->instrStream.count - 1;
}

//写入操作码
static void writeOpCode(CompileUnit *compileUnit, OpCode opCode) {
    writeByte(compileUnit, opCode);
    //累计需要的运行时栈空间大小
    compileUnit->stackSlotNum += opCodeSlotsUsed[opCode];
    //计算栈空间使用的峰值
    compileUnit->fn->maxStackSlotUsedNum = max(compileUnit->fn->maxStackSlotUsedNum, compileUnit->stackSlotNum);
}

//写入1个字节的操作数
static int writeByteOperand(CompileUnit *compileUnit, int operand) {
    return writeByte(compileUnit, operand);
}

//写入2个字节的操作数
static int writeShortOperand(CompileUnit *compileUnit, int operand) {
    //使用大端法，先写入高八位数据
    writeByteOperand(compileUnit, (operand >> 8) & 0xff);
    //再写入低八位
    writeByteOperand(compileUnit, operand & 0xff);
}

//写入操作数为1字节的指令
static int writeOpCodeByteOperand(CompileUnit *compileUnit, OpCode opCode, int operand) {
    writeOpCode(compileUnit, opCode);
    return writeByteOperand(compileUnit, operand);
}

//写入操作数为2字节的指令
static int writeOpCodeShortOperand(CompileUnit *compileUnit, OpCode opCode, int operand) {
    writeOpCode(compileUnit, opCode);
    return writeShortOperand(compileUnit, operand);
}

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


//编译程序的入口
static void compileProgram(CompileUnit *compileUnit) { ; }

//编译模块
ObjFn *compileModule(VM *vm, ObjModule *objModule, const char *moduleCode) {
    //为新的模块创建单独的词法分析器
    Parser parser;
    //初始化parser
    parser.parent = vm->curParser;
    vm->curParser = &parser;
    if (objModule->name == CORE_MODULE) {
        //如果是核心模块,使用core.script.inc初始化
        initParser(vm, &parser, "core.script.inc", moduleCode, objModule);
    } else {
        //一般模块
        initParser(vm, &parser, (const char *) objModule->name->value.start, moduleCode, objModule);
    }
    //创建编译单元并初始化
    CompileUnit compileUnit;
    initCompileUnit(&parser, &compileUnit, null, false);
    //记录模块变量的数量
    uint32_t moduleVarNum = objModule->moduleVarValue.count;
    //由于初始parser的curToken.type为UNKNOW，所以手动使其指向第一个合法Token
    getNextToken(&parser);
    while(!matchToken(&parser,TOKEN_EOF)){
        compileProgram(&compileUnit);
    }
    //后面还有很多要做的,临时放一句话在这提醒.
    //不过目前上面是死循环,本句无法执行。
    printf("There is something to do...\n");
    exit(0);
}

