#include <stdint.h>

//
// Created by yaser on 1/24/2020.
//
#include <stdio.h>

typedef struct Test{
    uint32_t name;
    uint64_t age;
    uint64_t age2;

    uint64_t age1;

    struct Test* age3;
} test;

int main() {
    printf("%lu", sizeof(test *));
    return 0;
}