//
// Created by wanyu on 2019/10/30.
//

#ifndef TEST_PARSER_H
#define TEST_PARSER_H

#include "common.h"
#include "vm.h"
#include "class.h"
#include "compiler.h"

#define resetCurTokenStart parser->curToken.start = parser->nextCharPtr - 1
typedef enum {
    TOKEN_UNKNOWN,
    // 数据类型

    TOKEN_NUM,           //数字
    TOKEN_STRING,           //字符串
    TOKEN_ID,               //变量名
    TOKEN_INTERPOLATION,     //内嵌表达式

    // 关键字(系统保留字)

    TOKEN_VAR,           //'var'
    TOKEN_FUN,           //'fun'
    TOKEN_IF,           //'if'
    TOKEN_ELSE,               //'else'
    TOKEN_TRUE,               //'true'
    TOKEN_FALSE,               //'false'
    TOKEN_WHILE,               //'while'
    TOKEN_FOR,               //'for'
    TOKEN_BREAK,               //'break'
    TOKEN_CONTINUE,         //'continue'
    TOKEN_RETURN,           //'return'
    TOKEN_NULL,               //'null'

    //以下是关于类和模块导入的token

    TOKEN_CLASS,               //'class'
    TOKEN_THIS,               //'this'
    TOKEN_STATIC,           //'static'
    TOKEN_IS,           // 'is'
    TOKEN_SUPER,               //'super'
    TOKEN_IMPORT,           //'import'

    //分隔符

    TOKEN_COMMA,           //','
    TOKEN_COLON,           //':'
    TOKEN_LEFT_PAREN,       //'('
    TOKEN_RIGHT_PAREN,       //')'
    TOKEN_LEFT_BRACKET,       //'['
    TOKEN_RIGHT_BRACKET,       //']'
    TOKEN_LEFT_BRACE,       //'{'
    TOKEN_RIGHT_BRACE,       //'}'
    TOKEN_DOT,           //'.'
    TOKEN_DOT_DOT,       //'..'

    //简单双目运算符

    TOKEN_ADD,           //'+'
    TOKEN_SUB,           //'-'
    TOKEN_MUL,           //'*'
    TOKEN_DIV,           //'/'
    TOKEN_MOD,           //'%'

    //赋值运算符

    TOKEN_ASSIGN,       //'='

    // 位运算符

    TOKEN_BIT_AND,       //'&'
    TOKEN_BIT_OR,       //'|'
    TOKEN_BIT_NOT,       //'~'
    TOKEN_BIT_SHIFT_RIGHT,  //'>>'
    TOKEN_BIT_SHIFT_LEFT,   //'<<'

    // 逻辑运算符

    TOKEN_LOGIC_AND,       //'&&'
    TOKEN_LOGIC_OR,       //'||'
    TOKEN_LOGIC_NOT,       //'!'

    //关系操作符

    TOKEN_EQUAL,           //'=='
    TOKEN_NOT_EQUAL,       //'!='
    TOKEN_GREATE,       //'>'
    TOKEN_GREATE_EQUAL,       //'>='
    TOKEN_LESS,           //'<'
    TOKEN_LESS_EQUAL,       //'<='

    TOKEN_QUESTION,       //'?'

    //文件结束标记,仅词法分析时使用

    TOKEN_EOF           //'EOF'
} TokenType;

typedef struct {
    TokenType type;
    const char *start;//该Token在代码中的起始位置
    uint32_t length;//Token的长度
    uint32_t lineNo;//Token所在的行号
    Value value; //Token的Value结构表示
} Token;

struct parser {
    const char *file;//源代码文件
    const char *sourceCode;//指向源码的指针
    const char *nextCharPtr;//指向下一个字符的指针
    char curChar;
    Token curToken;//当前Token
    Token preToken;//前一个Token
    ObjModule *curModule;//当前正在编译的模块
    CompileUnit *curCompileUnit;//当前编译单元
    int interpolationExpRightParenNum;//处于内嵌表达式内部，期望的右括号个数
    struct parser *parent;//指向父parser
    VM *vm;//当前parser的虚拟机
};
#define PEEK_TOKEN(parserPtr) parserPtr->curToken.type

char lookAheadChar(Parser *parser);

void getNextToken(Parser *parser);

bool matchToken(Parser *parser, TokenType expected);

void consumeCurToken(Parser *parser, TokenType expected, const char *errMsg);

void consumeNextToken(Parser *parser, TokenType expected, const char *errMsg);

uint32_t getByteNumOfEncodeUtf8(int value);

uint8_t encodeUtf8(uint8_t *buf, int value);

void initParser(VM *vm, Parser *parser, const char *file, const char *sourceCode, ObjModule *objModule);

#endif //TEST_PARSER_H
