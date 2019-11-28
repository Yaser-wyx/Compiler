//
// Created by wanyu on 2019/10/29.
//

#ifndef COMPLIER_UNICODEUTF8_H
#define COMPLIER_UNICODEUTF8_H

#include <stdint.h>

uint32_t getByteNumOfEncodeUtf8(int value);

uint32_t getByteNumOfDecodeUtf8(uint8_t byte);

uint8_t encodeUtf8(uint8_t *buf, int value);

int decodeUtf8(const uint8_t *bytePtr, uint32_t length);

#endif //COMPLIER_UNICODEUTF8_H
