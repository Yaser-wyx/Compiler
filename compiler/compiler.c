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
    struct compileUnit *enclosingUnit;//包含此编译单元的编译单元，也就是父编译单元
    Parser *curParser;//当前parser
};
typedef enum {
    BP_NONE,      //无绑定能力
    //从上往下,优先级越来越高

    BP_LOWEST,    //最低绑定能力
    BP_ASSIGN,    // =
    BP_CONDITION,   // ?:
    BP_LOGIC_OR,    // ||
    BP_LOGIC_AND,   // &&
    BP_EQUAL,      // == !=
    BP_IS,        // is
    BP_CMP,       // < > <= >=
    BP_BIT_OR,    // |
    BP_BIT_AND,   // &
    BP_BIT_SHIFT, // << >>
    BP_RANGE,       // ..
    BP_TERM,      // + -
    BP_FACTOR,      // * / %
    BP_UNARY,    // - ! ~
    BP_CALL,     // . () []
    BP_HIGHEST
} BindPower;   //定义了操作符的绑定权值,即优先级

//按照作用域划分的变量类型
typedef enum {
    VAR_SCOPE_INVALID,
    VAR_SCOPE_LOCAL,    //局部变量
    VAR_SCOPE_UPVALUE,  //upvalue
    VAR_SCOPE_MODULE    //模块变量
} VarScopeType;   //标识变量作用域

typedef struct {
    VarScopeType scopeType;//变量的作用域
    //根据scodeType的值,
    //此索引可能指向局部变量或upvalue或模块变量
    int index;
} Variable;//变量，只用于内部变量查找用



//指示符函数指针，用于指向各个符号的nud和led方法
typedef void(*DenotationFn)(CompileUnit *compileUnit, bool canAssign);

//签名函数指针
typedef void (*methodSignatureFn)(CompileUnit *cu, Signature *signature);

typedef struct {
    const char *id;//符号的字符串表示
    BindPower lbp;//左绑定权值，如果不关注左操作数的，该权值为0
    DenotationFn nud;   //字面量,变量,前缀运算符等不关注左操作数的Token调用的方法
    DenotationFn led;  //中缀运算符等关注左操作数的Token调用的方法
    //表示本符号在类中被视为一个方法.
    //为其生成一个方法签名.
    methodSignatureFn methodSign;
} SymbolBindRule;//符号绑定规则


//向当前编译单元的函数添加常量，并返回索引
static uint32_t addConstant(CompileUnit *compileUnit, Value constant) {
    ValueBufferAdd(compileUnit->curParser->vm, &compileUnit->fn->constants, constant);
    return compileUnit->fn->constants.count - 1;
}


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

//写入操作码，注意，操作码紧紧只是枚举类型数据，范围从0~255，不是具体的字符串
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
static void writeOpCodeShortOperand(CompileUnit *compileUnit, OpCode opCode, int operand) {
    writeOpCode(compileUnit, opCode);
    writeShortOperand(compileUnit, operand);
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

//生成加载常亮的指令
static void emitLoadConstant(CompileUnit *compileUnit, Value value) {
    int index = addConstant(compileUnit, value);
    writeOpCodeShortOperand(compileUnit, OPCODE_LOAD_CONSTANT, index);//加载指定索引位置的常量
}

//数字与字符串.nud() 编译字面量
static void literal(CompileUnit *compileUnit, bool canAssign UNUSED) {
    //literal是常量(数字和字符串)的nud方法,用来返回字面值.
    emitLoadConstant(compileUnit, compileUnit->curParser->preToken.value);
}

//把Signature转换为字符串,返回字符串长度
static uint32_t sign2String(Signature *sign, char *buf) {
    uint32_t pos = 0;
    //复制方法名xxx
    memcpy(buf + pos, sign->name, sign->length);
    pos += sign->length;

    //下面单独处理方法名之后的部分
    switch (sign->type) {
        //SIGN_GETTER形式:xxx,无参数,上面memcpy已完成
        case SIGN_GETTER:
            break;
            //SIGN_SETTER形式: xxx=(_),之前已完成xxx
        case SIGN_SETTER:
            buf[pos++] = '=';
            //下面添加=右边的赋值,只支持一个赋值
            buf[pos++] = '(';
            buf[pos++] = '_';
            buf[pos++] = ')';
            break;
            //SIGN_METHOD和SIGN_CONSTRUCT形式:xxx(_,...)
        case SIGN_CONSTRUCT:
        case SIGN_METHOD: {
            buf[pos++] = '(';
            uint32_t idx = 0;
            while (idx < sign->argNum) {
                buf[pos++] = '_';
                buf[pos++] = ',';
                idx++;
            }
            if (idx == 0) { //说明没有参数
                buf[pos++] = ')';
            } else { //用rightBracket覆盖最后的','
                buf[pos - 1] = ')';
            }
            break;
        }
            //SIGN_SUBSCRIPT形式:xxx[_,...]
        case SIGN_SUBSCRIPT: {
            buf[pos++] = '[';
            uint32_t idx = 0;
            while (idx < sign->argNum) {
                buf[pos++] = '_';
                buf[pos++] = ',';
                idx++;
            }
            if (idx == 0) { //说明没有参数
                buf[pos++] = ']';
            } else { //用rightBracket覆盖最后的','
                buf[pos - 1] = ']';
            }
            break;
        }
            //SIGN_SUBSCRIPT_SETTER形式:xxx[_,...]=(_)
        case SIGN_SUBSCRIPT_SETTER: {
            buf[pos++] = '[';
            uint32_t idx = 0;
            //argNum包括了等号右边的1个赋值参数,
            //这里是在处理等号左边subscript中的参数列表,因此减1.
            //后面专门添加该参数
            while (idx < sign->argNum - 1) {
                buf[pos++] = '_';
                buf[pos++] = ',';
                idx++;
            }
            if (idx == 0) { //说明没有参数
                buf[pos++] = ']';
            } else { //用rightBracket覆盖最后的','
                buf[pos - 1] = ']';
            }

            //下面为等号右边的参数构造签名部分
            buf[pos++] = '=';
            buf[pos++] = '(';
            buf[pos++] = '_';
            buf[pos++] = ')';
        }
    }

    buf[pos] = '\0';
    return pos;   //返回签名串的长度
}

//添加局部变量，并返回该局部变量的索引
static uint32_t addLocalVar(CompileUnit *cu, const char *name, uint32_t length) {
    ASSERT(cu->localVarNum < MAX_LOCAL_VAR_NUM, "localVarNums is lager than 128");
    localVar *localVar = &(cu->localVars[cu->localVarNum++]);//获得局部变量地址，并将局部变量的个数+1
    localVar->name = name;
    localVar->length = length;
    localVar->scopeDepth = cu->scopeDepth;
    localVar->isUpvalue = false;
    return cu->localVarNum;
}

//声明局部变量
static int declareLocalVar(CompileUnit *compileUnit, const char *name, uint32_t length) {
    //先判断局部变量的个数是否超过最大值
    if (compileUnit->localVarNum >= MAX_LOCAL_VAR_NUM) {
        COMPILE_ERROR(compileUnit->curParser, "the max length of local variable of one scope is %d",
                      MAX_LOCAL_VAR_NUM);
    }
    //判断该局部变量是否已被声明过了
    int idx = compileUnit->localVarNum - 1;
    while (idx >= 0) {
        LocalVar *localVar = &compileUnit->localVars[idx++];
        if (localVar->scopeDepth < compileUnit->scopeDepth) {
            //如果该局部变量的作用域小于当前作用域的位置
            break;
        }
        if (localVar->length == length && memcmp(localVar->name, name, length)) {
            //提前声明过了
            char idName[MAX_ID_LEN] = {'\0'};
            memcpy(idName, name, length);
            COMPILE_ERROR(compileUnit->curParser, "identifier \"%s\" redefinition!", idName);
        }
    }
    //完成判断
    return addLocalVar(compileUnit, name, length);//添加变量定义
}

//根据作用域，声明变量
static int declareVariable(CompileUnit *compileUnit, const char *name, uint32_t length) {
    if (compileUnit->scopeDepth == -1) {
        //当前为模块作用域
        int index = defineModuleVar(compileUnit->curParser->vm, compileUnit->curParser->curModule, name, length,
                                    VT_TO_VALUE(VT_NULL));
        if (index != -1) {
            //提前声明过了
            char idName[MAX_ID_LEN] = {'\0'};
            memcpy(idName, name, length);
            COMPILE_ERROR(compileUnit->curParser, "identifier \"%s\" redefinition!", idName);
        }
        return index;
    }
    return declareLocalVar(compileUnit, name, length);
}

//为单运算符方法创建签名
static void unaryMethodSignature(CompileUnit *compileUnit UNUSED, Signature *signature UNUSED) {
    //名称部分在调用前已经完成,只修改类型
    signature->type = SIGN_GETTER;
}

//为中缀运算符创建签名
static void infixMethodSignature(CompileUnit *compileUnit, Signature *signature) {
    signature->type = SIGN_METHOD;//中缀运算符就是方法
    signature->argNum = 1;
    consumeCurToken(compileUnit->curParser, TOKEN_LEFT_PAREN, "expect '(' after infix operator!");
    consumeCurToken(compileUnit->curParser, TOKEN_ID, "expect variable!");//读取参数
    declareVariable(compileUnit, compileUnit->curParser->preToken.start,
                    compileUnit->curParser->preToken.length);//为参数声明变量
    consumeCurToken(compileUnit->curParser, TOKEN_RIGHT_PAREN, "expect ')' after parameter!");
}

//为既做单运算符，又做中缀运算符的符号创建签名
static void mixMethodSignature(CompileUnit *compileUnit, Signature *signature) {
    if (matchToken(compileUnit->curParser, TOKEN_LEFT_PAREN)) {
        //如果能匹配到(,则说明是中缀
        infixMethodSignature(compileUnit, signature);
    } else {
        //否则就是前缀单运算
        unaryMethodSignature(compileUnit, signature);
    }
}

//不关注左操作数的符号称为前缀符号
//用于如字面量,变量名,前缀符号等非运算符
#define PREFIX_SYMBOL(nud) {NULL, BP_NONE, nud, NULL, NULL}

//前缀运算符,如'!'
#define PREFIX_OPERATOR(id) {id, BP_NONE, unaryOperator, NULL, unaryMethodSignature}

//关注左操作数的符号称为中缀符号
//数组'[',函数'(',实例与方法之间的'.'等
#define INFIX_SYMBOL(lbp, led) {NULL, lbp, NULL, led, NULL}

//中棳运算符
#define INFIX_OPERATOR(id, lbp) {id, lbp, NULL, infixOperator, infixMethodSignature}

//既可做前缀又可做中缀的运算符,如'-'
#define MIX_OPERATOR(id) {id, BP_TERM, unaryOperator, infixOperator, mixMethodSignature}

//占位用的
#define UNUSED_RULE {NULL, BP_NONE, NULL, NULL, NULL}

SymbolBindRule Rules[] = {
        /* TOKEN_INVALID*/            UNUSED_RULE,
        /* TOKEN_NUM	*/            PREFIX_SYMBOL(literal),
        /* TOKEN_STRING */            PREFIX_SYMBOL(literal),
};

//语法分析核心代码，是js中expression的c版本
static void expression(CompileUnit *compileUnit, BindPower rbp) {
    //以中缀运算符表达式"aSwTe"为例,
    //大写字符表示运算符,小写字符表示操作数
    //假设：进入expression时,curToken是操作数w, preToken是运算符S
    //获取操作数w的nud方法
    DenotationFn nud = Rules[compileUnit->curParser->curToken.type].nud;
    ASSERT(nud != null, "nud is null!");
    getNextToken(compileUnit->curParser);//前进一个Token，curToken变为T
    bool canAssign = rbp < BP_ASSIGN;//如果进入expression时运算符的优先级小于assign的绑定权值，则表示当前环境下是可以进行赋值操作
    nud(compileUnit, canAssign);//通过nud方法，无论当前是操作数还是运算符，都能保证获取读取一个操作数，然后curToken一定会是运算符
    while (rbp < Rules[compileUnit->curParser->curToken.type].lbp) {//比较两个运算符绑定权值的大小
        DenotationFn led = Rules[compileUnit->curParser->curToken.type].led;//获取T的led方法
        // 注：此时的token已经与一开始的token不一样了，且由于nud方法的缘故，此时一定是运算符token
        getNextToken(compileUnit->curParser);//当前curToken为e
        led(compileUnit, canAssign);//运行T的led方法
    }
}

//通过签名编译方法调用
static void emitCallBySignature(CompileUnit *compileUnit, Signature *sign, OpCode opCode) {
    ASSERT(sign->argNum <= 16, "args is lager than 16!");
    char signBuffer[MAX_SIGN_LEN];//定义一个缓冲区接受函数签名解析后的数据
    uint32_t length = sign2String(sign, signBuffer);//将签名解析为字符串格式，并返回长度
    int symbolIndex = ensureSymbolExist(compileUnit->curParser->vm, &compileUnit->curParser->vm->allMethodNames,
                                        signBuffer, length);
    writeOpCodeShortOperand(compileUnit, opCode + sign->argNum, symbolIndex);
    //此时在常量表中预创建一个空slot占位,将来绑定方法时再装入基类
    if (opCode == OPCODE_SUPER0) {
        writeShortOperand(compileUnit, addConstant(compileUnit, VT_TO_VALUE(VT_NULL)));
    }
}

//生成方法调用的指令，仅限callX指令
static void emitCall(CompileUnit *compileUnit, int numArgs, const char *name, int length) {
    int symbolIndex = ensureSymbolExist(compileUnit->curParser->vm, &compileUnit->curParser->vm->allMethodNames, name,
                                        length);
    writeOpCodeShortOperand(compileUnit, OPCODE_CALL0 + numArgs, symbolIndex);
}

//中缀运算符.led方法
static void infixOperator(CompileUnit *compileUnit, bool canAssign UNUSED) {
    //注：进入led或nud方法后，curToken已经变成了操作数，preToken才是当前方法的运算符
    SymbolBindRule *rule = &Rules[compileUnit->curParser->preToken.type];//获取当前方法的运算符绑定规则
    BindPower rbp = rule->lbp;//将lbp赋值给rbp
    expression(compileUnit, rbp);
    Signature signature = {SIGN_METHOD, rule->id, strlen(rule->id), 1};
    emitCallBySignature(compileUnit, &signature, OPCODE_CALL0);
}

//前缀运算符.nud方法, 如'-','!'等
static void unaryOperator(CompileUnit *compileUnit, bool canAssign UNUSED) {
    //注：进入led或nud方法后，curToken已经变成了操作数，preToken才是当前方法的运算符
    SymbolBindRule *rule = &Rules[compileUnit->curParser->preToken.type];
    //BP_UNARY做为rbp去调用expression解析右操作数
    expression(compileUnit, BP_UNARY);
    //生成调用前缀运算符的指令
    //0个参数,前缀运算符都是1个字符,长度是1
    emitCall(compileUnit, 0, rule->id, 1);
}

//声明模块变量，
static int declareModuleVar(VM *vm, ObjModule *objModule, const char *name, uint32_t length, Value value) {
    ValueBufferAdd(vm, &objModule->moduleVarValue, value);//添加模块变量的值
    return addSymbol(vm, &objModule->moduleVarName, name, length);//添加模块变量的名字
}

//获取距离当前编译单元最近的含有enclosingClassBK的父编译单元
static CompileUnit *getEnclosingClassBKUnit(CompileUnit *compileUnit) {
    while (compileUnit != null) {
        if (compileUnit->enclosingClassBK != null) {
            return compileUnit;
        }
        compileUnit = compileUnit->enclosingUnit;
    }
    return null;
}

//获取包含当前编译单元最近的classBookKeep
static ClassBookKeep *getEnclosingClassBK(CompileUnit *compileUnit) {
    CompileUnit *compileUnit1 = getEnclosingClassBKUnit(compileUnit);
    if (compileUnit1 != null) {
        return compileUnit1->enclosingClassBK;
    }
    return null;
}

//处理实参列表，并为其生成加载实参的指令
static void processArgList(CompileUnit *compileUnit, Signature *signature) {
    //由主调方保证参数不空
    ASSERT(cu->curParser->curToken.type != TOKEN_RIGHT_PAREN &&
           cu->curParser->curToken.type != TOKEN_RIGHT_BRACKET, "empty argument list!");
    do {
        signature->argNum++;
        if (signature->argNum > MAX_ARG_NUM) {
            COMPILE_ERROR(compileUnit->curParser, "the max number of argument is %d!",
                          MAX_ARG_NUM);
        }
        expression(compileUnit, BP_LOWEST);//读取参数，并生成指令
    } while (matchToken(compileUnit->curParser, TOKEN_COMMA));
}

//定义形参列表中的各个形参变量
static void processParaList(CompileUnit *compileUnit, Signature *signature) {
    ASSERT(cu->curParser->curToken.type != TOKEN_RIGHT_PAREN &&
           cu->curParser->curToken.type != TOKEN_RIGHT_BRACKET, "empty argument list!");
    do {
        signature->argNum++;
        if (signature->argNum > MAX_ARG_NUM) {
            COMPILE_ERROR(compileUnit->curParser, "the max number of argument is %d!",
                          MAX_ARG_NUM);
        }
        //消耗掉一个TOKEN，确保该Token是变量名
        consumeCurToken(compileUnit->curParser, TOKEN_ID, "excepted variable name!");
        //创建该变量
        declareVariable(compileUnit, compileUnit->curParser->preToken.start, compileUnit->curParser->preToken.length);
    } while (matchToken(compileUnit->curParser, TOKEN_COMMA));
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
    while (!matchToken(&parser, TOKEN_EOF)) {
        compileProgram(&compileUnit);
    }
    //后面还有很多要做的,临时放一句话在这提醒.
    //不过目前上面是死循环,本句无法执行。
    printf("There is something to do...\n");
    exit(0);
}

