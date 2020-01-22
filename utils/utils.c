#include "utils.h"
#include "vm.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

//内存管理三种功能:
//   1 申请内存
//   2 修改空间大小
//   3 释放内存
void *menManager(VM *vm, void *ptr, size_t oldSize, size_t newSize) {
    vm->allocatedBytes += (newSize - oldSize);
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, newSize);
}

// 找出大于等于v最近的2次幂
uint32_t ceilToPowerOf2(uint32_t v) {
    //思想：在二进制层面上，把V最高位后面的0全部变成1，再加1，就是大于等于v最近的2次幂
    v += (v == 0);  //修复当v等于0时结果为0的边界情况
    v--;
    for (int i = 1; i <= 16; i <<= 1) {
        v |= v >> i;
    }
    v++;
    return v;
}

DEFINE_BUFFER_METHOD(String)

DEFINE_BUFFER_METHOD(Int)

DEFINE_BUFFER_METHOD(Char)

DEFINE_BUFFER_METHOD(Byte)

void symbolTableClear(VM *vm, SymbolTable *buffer) {
    uint32_t idx = 0;
    while (idx < buffer->count) {
        memManager(vm, buffer->datas[idx++].str, 0, 0);
    }
    StringBufferClear(vm, buffer);
}

//通用报错函数
void errorReport(void *parser, ErrorType errorType, const char *fmt, ...) {
    char buffer[DEFAULT_BUfFER_SIZE] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, DEFAULT_BUfFER_SIZE, fmt, ap);
    va_end(ap);

    switch (errorType) {
        case ERROR_IO:
        case ERROR_MEM:
            fprintf(stderr, "%s:%d In function %s():%s\n",
                    __FILE__, __LINE__, __func__, buffer);
            break;
        case ERROR_LEX:
        case ERROR_COMPILE:
            ASSERT(parser != NULL, "parser is null!");
            fprintf(stderr, "%s:%d \"%s\"\n", ((Parser *) parser)->file,
                    ((Parser *) parser)->preToken.lineNo, buffer);
            break;
        case ERROR_RUNTIME:
            fprintf(stderr, "%s\n", buffer);
            break;
        default:
            NOT_REACHED();
    }
    exit(1);
}

//二进制转十进制
uint32_t bin2dec(char *bin) {
    int len = strlen(bin) - 1;
    int index = 0;
    uint32_t value = 0;
    while (index < len) {
        if (bin[index] == '1') {
            value += pow(2, len - index);
        }
        index++;
    }
    return value;
}