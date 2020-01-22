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

struct keywordToken {
    char *keyword;
    uint32_t length;
    TokenType tokenType;
};

struct keywordToken keywordTokenTable[] = {
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

//parse the word which in start, if is the keyword then return the corresponding token, otherwise return TOKEN_ID.
static TokenType idOrKeyword(const char *start, uint32_t length) {
    for (int i = 0; i < len(keywordTokenTable); ++i) {
        if (keywordTokenTable[i].length == length) {
            if (memcmp(keywordTokenTable[i].keyword, start, length) == 0) {
                return keywordTokenTable[i].tokenType;
            }
        }
    }
    return TOKEN_ID;
}

//let parser look ahead a char.
char lookAheadChar(Parser *parser) {
    return *parser->nextCharPtr;
}


static void getNextChar(Parser *parser) {
    parser->curChar = *parser->nextCharPtr;
    parser->nextCharPtr++;
}

static bool matchNextChar(Parser *parser, char expChar) {
    if (lookAheadChar(parser) == expChar) {
        getNextChar(parser);
        return true;
    }
    return false;
}

static void skipSpace(Parser *parser) {
    while (isspace(parser->curChar)) {
        //todo test \n
        if (parser->curChar == '\n') {
            parser->curToken.lineNo++;
        }
        getNextChar(parser);
    }
}

inline bool isLegalSymbol(const char c) {
    return isalnum(c) || c == '_';
}

//read the whole symbol, determine its length and type
static void parseId(Parser *parser, TokenType type) {
    while (isLegalSymbol(parser->curChar)) {
        //if is legal, then get next char;
        getNextChar(parser);
    }
    Token *curToken = &parser->curToken;
    uint32_t length = curToken->length = (parser->nextCharPtr - curToken->start - 1);//set the symbol length
    if (type == TOKEN_UNKNOWN) {
        //if unknown, then identify its type
        type = idOrKeyword(curToken->start, length);
    }
    curToken->type = type;
}

//parse the unicode code point, and transform this to utf-8, then write the parsed value to buffer
static void parserUnicodeCodePoint(Parser *parser, ByteBuffer *buffer) {
    int index = 0;
    int digital = 0;
    int value = 0;
    while (index++ < 4) {
        //the length of code point is less than 4
        getNextChar(parser);
        if (parser->curChar == '\0') {
            LEX_ERROR(parser, "unterminated unicode!");
        }
        if (parser->curChar >= '0' && parser->curChar <= '9') {
            digital = parser->curChar - '0';
        } else if (parser->curChar >= 'a' && parser->curChar <= 'f') {
            digital = parser->curChar - 'a' + 10;
        } else if (parser->curChar >= 'A' && parser->curChar <= 'F') {
            digital = parser->curChar - 'A' + 10;
        }
        value = value * 16 | digital;
    }
    uint32_t byteNum = getByteNumOfEncodeUtf8(value);
    ASSERT(byteNum != 0, "utf8 code bytes should be between 1 and 4!");
    ByteBufferFillWrite(parser->vm, buffer, 0, byteNum);
    encodeUtf8(buffer->datas + buffer->count - byteNum, value);
}

static void parserString(Parser *parser) {
    ByteBuffer buffer;
    ByteBufferInit(&buffer);
    while (true) {
        getNextChar(parser);
        if (parser->curChar == '\0') {
            LEX_ERROR(parser, "unterminated String");
            break;
        }
        if (parser->curChar == '"') {
            parser->curToken.type = TOKEN_STRING;
            break;
        }
        if (parser->curChar == '%') {
            if (!matchNextChar(parser, '(')) {
                LEX_ERROR(parser, "In string, '%' must followed by '('!");
            }
            if (parser->interpolationExpRightParenNum != 0) {
                COMPILE_ERROR(parser, "sorry we don't support nest interpolate expression!");
            }
            parser->interpolationExpRightParenNum = 1;
            parser->curToken.type = TOKEN_INTERPOLATION;
            break;
        }
        //handle ESC
        if (parser->curChar == '\\') {
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
                case '\\':
                    ByteBufferAdd(parser->vm, &buffer, '\\');
                    break;
                case 'u':
                    parserUnicodeCodePoint(parser, &buffer);
                    break;
                case '"':
                    ByteBufferAdd(parser->vm, &buffer, '"');
                    break;
                default:
                    LEX_ERROR(parser, "unSupport ESC \\%c", parser->curChar);
                    break;
            }
        } else {
            //normal string
            ByteBufferAdd(parser->vm, &buffer, parser->curChar);
        }
    }
    //clear buffer.
    ByteBufferClear(parser->vm, &buffer);
}

static void skipAline(Parser *parser) {
    while (parser->curChar != '\0') {
        if (parser->curChar == '\n') {
            getNextChar(parser);
            parser->curToken.lineNo++;
            break;
        } else {
            getNextChar(parser);
        }
    }
}

static void skipComment(Parser *parser) {
    getNextChar(parser);
    char nextChar = lookAheadChar(parser);
    if (parser->curChar == '/') {
        //line comment
        skipAline(parser);
        goto clean_space;
    } else {
        //block comment
        while (nextChar != '\0') {
            //match the '*/' until the end of file.
            getNextChar(parser);
            if (parser->curChar == '\n') {
                parser->curToken.lineNo++;
            } else if (parser->curChar == '*') {
                //match next char, if next char is '/', then return, otherwise continue match the rest of file.
                if (matchNextChar(parser, '/')) {
                    goto clean_space;
                }
            }
            nextChar = lookAheadChar(parser);//update nextChar
        }
        //if not match the '*/' in file, then report the lex error.
        LEX_ERROR(parser, "expect '*/' before file end!");
    }
    //maybe there's a blank after the comment
    clean_space:
    skipSpace(parser);
}