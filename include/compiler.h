//
// Created by yaser on 1/27/2020.
//

#ifndef TEMP_COMPILER_H
#define TEMP_COMPILER_H

#include "obj_fn.h"

#define MAX_LOCAL_VAR_NUM 128  //作用域中局部变量的最大个数
#define MAX_UPVALUE_NUM 128
#define MAX_ID_LEN 128   //变量名最大长度

#define MAX_METHOD_NAME_LEN MAX_ID_LEN
#define MAX_ARG_NUM 16 //最大参数个数

//函数名长度+'('+n个参数+(n-1)个参数分隔符','+')'
//方法签名的最大长度
#define MAX_SIGN_LEN MAX_METHOD_NAME_LEN + MAX_ARG_NUM * 2 + 1

#define MAX_FIELD_NUM 128 //域中属性的最大数量

typedef struct {
    //表示当前upvalue是否为外层函数的局部变量
    bool isEnclosingLocalVar;
    //如果当前upvalue为外层函数的局部变量，则index为外层函数的局部变量索引，否则为外层函数的upvalue索引
    uint32_t index;
} Upvalue;//upvalue结构

typedef struct {
    const char *name;//局部变量名
    uint32_t length;//变量名长度

    int scopeDepth;//所处作用域层次

    //表示该局部变量是否被内层函数所引用。
    bool isUpvalue;
} localVar;

typedef enum {
    SIGN_CONSTRUCT,  //构造函数
    SIGN_METHOD,  //普通方法
    SIGN_GETTER, //getter方法
    SIGN_SETTER, //setter方法
    SIGN_SUBSCRIPT, //getter形式的下标
    SIGN_SUBSCRIPT_SETTER   //setter形式的下标
} SignatureType;   //方法的签名

typedef struct {
    SignatureType type;//签名类型
    const char *name;//签名
    uint32_t length;//签名的长度
    uint32_t argNum;//签名的个数
} Signature;//签名结构，方法声明的字符串形式

typedef struct loop {
    int condStartIndex;//循环条件地址
    int bodyStartIndex;//循环体起始地址
    int scopeDepth;//循环体内部作用域深度
    int exitIndex;//循环退出地址
    struct loop *enclosingLoop;//外层循环
} Loop;

typedef struct {
    ObjString *name;          //类名
    SymbolTable fields;          //类属性符号表
    bool inStatic;          //若当前编译静态方法就为真
    IntBuffer instantMethods;  //实例方法
    IntBuffer staticMethods;   //静态方法
    Signature *signature;      //当前正在编译的签名
} ClassBookKeep;    //用于记录类编译时的信息

typedef struct compileUnit CompileUnit;

int defineModuleVar(VM *vm, ObjModule *objModule, const char *name, uint32_t length, Value value);

#endif //TEMP_COMPILER_H
