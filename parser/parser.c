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

//match the next char, if match success, then read it and return true, else return false.
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

bool isLegalSymbol(const char c);

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

static void parseString(Parser *parser) {
    ByteBuffer str;
    ByteBufferInit(&str);
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
                    ByteBufferAdd(parser->vm, &str, '\0');
                    break;
                case 'a':
                    ByteBufferAdd(parser->vm, &str, '\a');
                    break;
                case 'b':
                    ByteBufferAdd(parser->vm, &str, '\b');
                    break;
                case 'f':
                    ByteBufferAdd(parser->vm, &str, '\f');
                    break;
                case 'n':
                    ByteBufferAdd(parser->vm, &str, '\n');
                    break;
                case 'r':
                    ByteBufferAdd(parser->vm, &str, '\r');
                    break;
                case 't':
                    ByteBufferAdd(parser->vm, &str, '\t');
                    break;
                case '\\':
                    ByteBufferAdd(parser->vm, &str, '\\');
                    break;
                case 'u':
                    parserUnicodeCodePoint(parser, &str);
                    break;
                case '"':
                    ByteBufferAdd(parser->vm, &str, '"');
                    break;
                default:
                    LEX_ERROR(parser, "unSupport ESC \\%c", parser->curChar);
                    break;
            }
        } else {
            //normal string
            ByteBufferAdd(parser->vm, &str, parser->curChar);
        }
    }
    ObjString *objString = newObjString(parser->vm, (const char *) str.datas, str.count);
    parser->curToken.value = OBJ_TO_VALUE(objString);
    //clear str buffer.
    ByteBufferClear(parser->vm, &str);
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

//parse hex number
static void parseHexNum(Parser *parser) {
    while (isxdigit(parser->curChar)) {
        getNextChar(parser);
    }
}

//parse dec number
static void parseDecNum(Parser *parser) {
    while (isdigit(parser->curChar)) {
        getNextChar(parser);
    }

    if (parser->curChar == '.' && isdigit(lookAheadChar(parser))) {
        getNextChar(parser);
        while (isdigit(parser->curChar)) {
            getNextChar(parser);
        }
    }
}

//parse oct number
static void parseOctNum(Parser *parser) {
    while (parser->curChar >= '0' && parser->curChar < '8') {
        getNextChar(parser);
    }
}

//parse number, include hex, dec and oct number.
//only support for the prefix, not support for the suffix
static void parseNum(Parser *parser) {
    if (parser->curChar == '0' && matchNextChar(parser, 'x')) {
        getNextChar(parser);//skip the x.
        parseHexNum(parser);
        parser->curToken.value = NUM_TO_VALUE(strtol(parser->curToken.start, null, 16));
    } else if (parser->curChar == '0') {
        char next = lookAheadChar(parser);
        if (next >= '0' && next < '8') {
            parseOctNum(parser);
            parser->curToken.value = NUM_TO_VALUE(strtol(parser->curToken.start, null, 8));
        }
    } else {
        parseDecNum(parser);
        parser->curToken.value = NUM_TO_VALUE(strtod(parser->curToken.start, null));
    }
    //now curChar is point to the first invalid char, and the nextCharPtr point to the second, so need minus 1
    parser->curToken.length = parser->nextCharPtr - parser->curToken.start - 1;
    parser->curToken.type = TOKEN_NUM;
}

//only recognize one token, and set the curToken to this.
void getNextToken(Parser *parser) {
    //set preToken as curToken
    parser->preToken = parser->curToken;
    skipSpace(parser);//skip the space before the word to be recognized.
    //init curToken
    parser->curToken.type = TOKEN_EOF;
    parser->curToken.length = 0;
    parser->curToken.value = VT_TO_VALUE(VT_UNDEFINED);
    resetCurTokenStart;
    while (parser->curChar != '\0') {
        //matching curChar, and set curToken type
        switch (parser->curChar) {
            case ',':
                parser->curToken.type = TOKEN_COMMA;
                break;
            case ':':
                parser->curToken.type = TOKEN_COLON;
                break;
            case '(':
                if (parser->interpolationExpRightParenNum > 0) {
                    parser->interpolationExpRightParenNum++;
                }
                parser->curToken.type = TOKEN_LEFT_PAREN;
                break;
            case ')':
                if (parser->interpolationExpRightParenNum > 0) {
                    parser->interpolationExpRightParenNum--;
                    if (parser->interpolationExpRightParenNum == 0) {
                        parseString(parser);
                        break;
                    }
                }
                parser->curToken.type = TOKEN_RIGHT_PAREN;
                break;
            case '[':
                parser->curToken.type = TOKEN_LEFT_BRACKET;
                break;
            case ']':
                parser->curToken.type = TOKEN_RIGHT_BRACKET;
                break;
            case '{':
                parser->curToken.type = TOKEN_LEFT_BRACE;
                break;
            case '}':
                parser->curToken.type = TOKEN_RIGHT_BRACE;
                break;
            case '.':
                if (matchNextChar(parser, '.')) {
                    parser->curToken.type = TOKEN_DOT_DOT;
                } else {
                    parser->curToken.type = TOKEN_DOT;
                }
                break;
            case '=':
                if (matchNextChar(parser, '=')) {
                    parser->curToken.type = TOKEN_EQUAL;
                } else {
                    parser->curToken.type = TOKEN_ASSIGN;
                }
                break;
            case '+':
                parser->curToken.type = TOKEN_ADD;
                break;
            case '-':
                parser->curToken.type = TOKEN_SUB;
                break;
            case '*':
                parser->curToken.type = TOKEN_MUL;
                break;
            case '/':
                if (matchNextChar(parser, '/') || matchNextChar(parser, '*')) {
                    //comment
                    skipComment(parser);
                    resetCurTokenStart;
                    continue;
                } else {
                    parser->curToken.type = TOKEN_DIV;
                }
                break;
            case '%':
                parser->curToken.type = TOKEN_MOD;
                break;
            case '&':
                if (matchNextChar(parser, '&')) {
                    parser->curToken.type = TOKEN_LOGIC_AND;
                } else {
                    parser->curToken.type = TOKEN_BIT_AND;
                }
                break;
            case '|':
                if (matchNextChar(parser, '|')) {
                    parser->curToken.type = TOKEN_LOGIC_OR;
                } else {
                    parser->curToken.type = TOKEN_BIT_OR;
                }
                break;
            case '~':
                parser->curToken.type = TOKEN_BIT_NOT;
                break;
            case '?':
                parser->curToken.type = TOKEN_QUESTION;
                break;
            case '>':
                if (matchNextChar(parser, '=')) {
                    parser->curToken.type = TOKEN_GREATE_EQUAL;
                } else if (matchNextChar(parser, '>')) {
                    parser->curToken.type = TOKEN_BIT_SHIFT_RIGHT;
                } else {
                    parser->curToken.type = TOKEN_GREATE;
                }
                break;
            case '<':
                if (matchNextChar(parser, '=')) {
                    parser->curToken.type = TOKEN_LESS_EQUAL;
                } else if (matchNextChar(parser, '<')) {
                    parser->curToken.type = TOKEN_BIT_SHIFT_LEFT;
                } else {
                    parser->curToken.type = TOKEN_LESS;
                }
                break;
            case '!':
                if (matchNextChar(parser, '=')) {
                    parser->curToken.type = TOKEN_NOT_EQUAL;
                } else {
                    parser->curToken.type = TOKEN_LOGIC_NOT;
                }
                break;
            case '"':
                parseString(parser);
                break;
            default:
                //used to match symbol and number.
                if (isalpha(parser->curChar) || parser->curChar == '_') {
                    //identify var
                    parseId(parser, TOKEN_UNKNOWN);
                } else if (isdigit(parser->curChar)) {
                    //parse digital
                    parseNum(parser);
                } else {
                    if (parser->curChar == '#' && matchNextChar(parser, '!')) {
                        skipAline(parser);
                        resetCurTokenStart;
                        continue;
                    }
                    LEX_ERROR(parser, "unsupport char: \'%c\', quit.", parser->curChar);
                }
                return;
        }
        //set length
        parser->curToken.length = (uint32_t) (parser->nextCharPtr - parser->curToken.start);
        getNextChar(parser);//set curChar to next is in order to the next token recognize.
        //finish match
        return;
    }
}

//read now token if matched, and return true
bool matchToken(Parser *parser, TokenType expected) {
    if (parser->curToken.type == expected) {
        getNextToken(parser);
        return true;
    }
    return false;
}

//assert curToken is expected, otherwise report error!
void consumeCurToken(Parser *parser, TokenType expected, const char *errMsg) {
    if (parser->curToken.type != expected) {
        COMPILE_ERROR(parser, errMsg);
    }
    getNextToken(parser);
}

//assert nextToken is expected, otherwise report error!
void consumeNextToken(Parser *parser, TokenType expected, const char *errMsg) {
    getNextToken(parser);
    if (parser->curToken.type != expected) {
        COMPILE_ERROR(parser, errMsg);
    }
}


void initParser(VM *vm, Parser *parser, const char *file, const char *sourceCode, ObjModule *objModule) {
    parser->file = file;
    parser->sourceCode = sourceCode;
    parser->curChar = *sourceCode;
    parser->nextCharPtr = sourceCode + 1;
    parser->curToken.lineNo = 1;
    parser->curToken.type = TOKEN_UNKNOWN;
    parser->curToken.start = null;
    parser->curToken.length = 0;
    parser->interpolationExpRightParenNum = 0;
    parser->vm = vm;
    parser->preToken = parser->curToken;
    parser->curModule = objModule;
}