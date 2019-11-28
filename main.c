//
// Created by wanyu on 2019/10/29.
//
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define mode_10 "10000000"
#define mode_6_1 "111111"

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

//返回value按照utf8编码后的字节数
uint32_t getByteNumOfEncodeUTF8(int value) {
    if (value <= 0x7f) {
        return 1;
    }
    if (value <= 0x7ff) {
        return 2;
    }
    if (value <= 0xffff) {
        return 3;
    }
    if (value <= 0x10ffff) {
        return 4;
    }
    return 0;
}

uint8_t encodeUTF8(uint8_t *buf, int value) {
    uint8_t size = getByteNumOfEncodeUTF8(value);
    switch (size) {
        case 1:
            *buf = value;
            break;
        case 2:
            *buf++ = 0xc0 | ((value >> 6) & 0x1f);//截取高字节
            *buf = 0x80 | (value & 0x3f);
            break;
        case 3:
            *buf++ = 0xe0 | ((value >> 12) & 0xf);
            *buf++ = 0x80 | ((value >> 6) & 0x3f);
            *buf = 0x80 | ((value) & 0x3f);
            break;
        case 4:
            *buf++ = 0xf0 | ((value >> 18) & 0x7);
            *buf++ = 0x80 | ((value >> 12) & 0x3f);
            *buf++ = 0x80 | ((value >> 6) & 0x3f);
            *buf = 0x80 | ((value) & 0x3f);
            break;
        default:
            break;
    }
    return size;
}

uint8_t encodeUtf8(uint8_t *buf, int value) {
    //按照大端字节序写入缓冲区
    if (value <= 0x7f) {   // 单个ascii字符需要1字节
        *buf = value & 0x7f;
        return 1;
    } else if (value <= 0x7ff) {  //此范围内数值编码为utf8需要2字节
        //先写入高字节
        *buf++ = 0xc0 | ((value & 0x7c0) >> 6);
        // 再写入低字节
        *buf = 0x80 | (value & 0x3f);
        return 2;
    } else if (value <= 0xffff) { //此范围内数值编码为utf8需要3字节
        // 先写入高字节
        *buf++ = 0xe0 | ((value & 0xf000) >> 12);
        //再写入中间字节
        *buf++ = 0x80 | ((value & 0xfc0) >> 6);
        //最后写入低字节
        *buf = 0x80 | (value & 0x3f);
        return 3;
    } else if (value <= 0x10ffff) { //此范围内数值编码为utf8需要4字节
        *buf++ = 0xf0 | ((value & 0x1c0000) >> 18);
        *buf++ = 0x80 | ((value & 0x3f000) >> 12);
        *buf++ = 0x80 | ((value & 0xfc0) >> 6);
        *buf = 0x80 | (value & 0x3f);
        return 4;
    }

    return 0;
}

void test(uint32_t start, uint32_t end) {
    uint8_t *bufA = malloc(100);
    uint8_t *bufB = malloc(100);
    for (uint32_t i = start; i <= end; ++i) {
        memset(bufA, 0, 100);
        memset(bufB, 0, 100);
        encodeUTF8(bufA, i);
        encodeUtf8(bufB, i);
        if (strcmp((const char *) bufA, (const char *) bufB) != 0) {
            printf("error: %d\n", i);
            printf("bufA:%s\n", bufA);
            printf("bufB:%s\n", bufB);
            return;
        }
    }
}

#define len(array) sizeof(array)/sizeof(array[0])

struct stu {
    int a;
    int b;
    uint32_t o;
    char *cc;
};

int main() {
    char c = '\t';
    printf("%d",isspace(c));
    return 1;
}