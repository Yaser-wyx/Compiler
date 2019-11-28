//
// Created by wanyu on 2019/10/30.
//

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "utils.h"
#include "unicodeUTF8.h"
#include <string.h>
#include <ctype.h>

//关键字结构
struct keywordToken {
    char *keyword;//关键字
    uint8_t length;//关键字长度
    TokenType token;//关键字token类型
};
//关键字查找表
struct keywordToken keywordsToken[] = {
        {"var",      3, TOKEN_VAR},
        {"fun",      3, TOKEN_FUN},
        {"if",       2, TOKEN_IF},
        {"else",     4, TOKEN_ELSE},
        {"true",     4, TOKEN_TRUE},
        {"false",    5, TOKEN_FALSE},
        {"while",    5, TOKEN_WHILE},
        {"for",      3, TOKEN_FOR},
        {"break",    5, TOKEN_BREAK},
        {"continue", 8, TOKEN_CONTINUE},
        {"return",   6, TOKEN_RETURN},
        {"null",     4, TOKEN_NULL},
        {"class",    5, TOKEN_CLASS},
        {"is",       2, TOKEN_IS},
        {"static",   6, TOKEN_STATIC},
        {"this",     4, TOKEN_THIS},
        {"super",    5, TOKEN_SUPER},
        {"import",   6, TOKEN_IMPORT},
        {NULL,       0, TOKEN_UNKNOWN}
};

//判断以start为起始的字符串是否为关键字，并返回相应的token
static TokenType idOrKeyword(const char *start, uint32_t length) {
    for (int index = 0; index < len(keywordsToken); ++index) {
        if (keywordsToken[index].length == length) {
            if (strcmp(keywordsToken[index].keyword, start)) {
                return keywordsToken[index].token;
            }
        }
    }
    return TOKEN_ID;//如果不是关键字，怎返回该字符串是变量
}

//读入下一个字符
inline char lookAheadChar(Parser *parser) {
    return *parser->nextCharPtr;
}

//将当前字符改为下一个字符
inline void getNextChar(Parser *parser) {
    parser->curChar = *parser->nextCharPtr++;
}

//查看下一个字符是否是期望的字符，如果是，则读取，并返回true
static bool matchNextChar(Parser *parser, char expectedChar) {
    if (lookAheadChar(parser) == expectedChar) {
        //符合预期
        getNextChar(parser);
        return true;
    }
    return false;
}

//跳过连续的空白
static void skipBlanks(Parser *parser) {
    while (isspace(parser->curChar)) {
        if (parser->curChar == '\n') {
            parser->curToken.lineNo++;
        }
        getNextChar(parser);
    }
}

//解析标识符
static void parseId(Parser *parser, TokenType type) {
    while (isalnum(parser->curChar) || parser->curChar == '_') {
        //连续读入多个字符
        getNextChar(parser);
    }
    size_t len = parser->nextCharPtr - parser->curToken.start - 1;
    if (type == TOKEN_UNKNOWN) {
        parser->curToken.type = idOrKeyword(parser->curToken.start, len);
    } else {
        parser->curToken.type = type;
    }
    parser->curToken.length = len;
}

//解析unicode码点
static void parseUnicodeCodePoint(Parser *parser, ByteBuffer *buf) {
    uint32_t idx = 0;
    int value = 0;
    uint8_t digit = 0;

//获取数值,u后面跟着4位十六进制数字
    while (idx++ < 4) {
        getNextChar(parser);
        if (parser->curChar == '\0') {
            LEX_ERROR(parser, "unterminated unicode!");
        }
        if (parser->curChar >= '0' && parser->curChar <= '9') {
            digit = parser->curChar - '0';
        } else if (parser->curChar >= 'a' && parser->curChar <= 'f') {
            digit = parser->curChar - 'a' + 10;
        } else if (parser->curChar >= 'A' && parser->curChar <= 'F') {
            digit = parser->curChar - 'A' + 10;
        } else {
            LEX_ERROR(parser, "invalid unicode!");
        }
        value = value * 16 | digit;
    }

    uint32_t byteNum = getByteNumOfEncodeUtf8(value);
    ASSERT(byteNum != 0, "utf8 encode bytes should be between 1 and 4!");

    //为代码通用, 下面会直接写buf->datas,在此先写入byteNum个0,以保证事先有byteNum个空间
    ByteBufferFillWrite(parser->vm, buf, 0, byteNum);

    //把value编码为utf8后写入缓冲区buf
    encodeUtf8(buf->datas + buf->count - byteNum, value);
}

//解析字符串
static void parseString(Parser *parser) {
    ByteBuffer buffer;
    ByteBufferInit(&buffer);
    while (true) {
        getNextChar(parser);
        if (parser->curChar == '\0') {
            LEX_ERROR(parser, "unterminated string!");
        }
        if (parser->curChar == '"') {
            parser->curToken.type = TOKEN_STRING;
            break;
        }
        if (parser->curChar == '%') {
            if (!matchToken(parser, '(')) {
                LEX_ERROR(parser, "'%' should followed by '('!");
            }
            if (parser->interpolationExpectRightParenNum > 0) {
                COMPILE_ERROR(parser, "sorry, I don`t support nest interpolate expression!");
            }
            parser->curToken.type = TOKEN_INTERPOLATION;
            parser->interpolationExpectRightParenNum = 1;
            break;
        }
        if (parser->curChar == '\\') {
            //处理转义字符
            getNextChar(parser);
            switch (parser->curChar) {
                case '0':
                    ByteBufferAdd(parser->vm, &buffer, '\0');
                    break;
                case 'a':
                    ByteBufferAdd(parser->vm, &buffer, '\a');
                    break;
                case 'b':
                    ByteBufferAdd(parser->vm, &buffer, '\b');
                    break;
                case 'f':
                    ByteBufferAdd(parser->vm, &buffer, '\f');
                    break;
                case 'n':
                    ByteBufferAdd(parser->vm, &buffer, '\n');
                    break;
                case 'r':
                    ByteBufferAdd(parser->vm, &buffer, '\r');
                    break;
                case 't':
                    ByteBufferAdd(parser->vm, &buffer, '\t');
                    break;
                case 'u':
                    parseUnicodeCodePoint(parser, &buffer);
                    break;
                case '"':
                    ByteBufferAdd(parser->vm, &buffer, '"');
                    break;
                case '\\':
                    ByteBufferAdd(parser->vm, &buffer, '\\');
                    break;
                default:
                    LEX_ERROR(parser, "unsupport escape \\%c", parser->curChar);
                    break;
            }
        } else {
            ByteBufferAdd(parser->vm, &buffer, parser->curChar);
        }
    }
    ByteBufferClear(parser->vm, &buffer);
}

static void skipAline(Parser *parser) {
    getNextChar(parser);
    while (parser->curChar != '\0') {
        if (parser->curChar == '\n') {
            parser->curToken.lineNo++;
            getNextChar(parser);
            break;
        }
        getNextChar(parser);
    }
}


