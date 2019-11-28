//
// Created by wanyu on 2019/10/29.
//
#include "unicodeUTF8.h"
#include "common.h"
#include "utils.h"

#define mode_10 "10000000"
#define mode_6_1 "111111"

//返回value按照utf8编码后的字节数
uint32_t getByteNumOfEncodeUtf8(int value) {
    ASSERT(value > 0, "Can`t encode negative value!");
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

//把value编码为utf8后写入buf，返回写入的字节数
uint8_t encodeUtf8(uint8_t *buf, int value) {
    uint8_t size = getByteNumOfEncodeUtf8(value);
    switch (size) {
        case 1:
            *buf = value;
            break;
        case 2:
            *buf++ = 0xc0 | ((value >> 6) & 0x1f);//截取高字节
            *buf = 0x80 | (value & 0x3f);
            break;
        case 3:
            *buf++ = 0xe0 | ((value >> 12) & 0x0f);
            *buf++ = 0x80 | ((value >> 6) & 0x3f);
            *buf = 0x80 | ((value) & 0x3f);
            break;
        case 4:
            *buf++ = 0xf0 | ((value >> 18) & 0x07);
            *buf++ = 0x80 | ((value >> 12) & 0x3f);
            *buf++ = 0x80 | ((value >> 6) & 0x3f);
            *buf = 0x80 | ((value) & 0x3f);
            break;
        default:
            NOT_REACHED()
            break;
    }
    return size;
}

//返回解码UTF8的字节数
uint32_t getByteNumOfDecodeUtf8(uint8_t byte) {
    //byte为utf8最高1字节
    if ((byte & 0xc0) == 0x80) {
        //指向低字节
        return 0;
    }
    if ((byte & 0xe0) == 0xc0) {
        return 2;
    }
    if ((byte & 0xf0) == 0xe0) {
        return 3;
    }
    if ((byte & 0xf8) == 0xf0) {
        return 4;
    }
    return 1;
}

//解码以bytePtr为起始地址的UTF-8序列 其最大长度为length 若不是UTF-8序列就返回-1
int decodeUtf8(const uint8_t *bytePtr, uint32_t length) {
    uint32_t remainLen = getByteNumOfDecodeUtf8(*bytePtr);//获取长度数据
    if (remainLen == 1) {
        return *bytePtr;//单字节直接返回
    }
    if (length < remainLen || remainLen == 0) {
        return -1;//数据流被截断了，不完整，或者bytePtr指向的是低字节
    }
    //是可以解析的数据
    int value = 0;
    //统一处理高字节
    switch (remainLen) {
        case 2:
            value |= *bytePtr & 0x1f;
            break;
        case 3:
            value |= *bytePtr & 0x0f;
            break;
        case 4:
            value |= *bytePtr & 0x07;
            break;
    }
    remainLen--;
    //统一处理低字节
    while (remainLen--) {
        bytePtr++;
        if ((*bytePtr & 0xc0) != 0x80) {
            return -1;
        }
        value = value << 6 | (*bytePtr & 0x3f);
    }
    return value;
}