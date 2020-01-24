#include <stdint.h>

//
// Created by yaser on 1/24/2020.
//
#include <stdio.h>

typedef struct Test {
    uint32_t name;
    uint64_t age;
    uint64_t age2;

    uint64_t age1;

    struct Test *age3;
} test;

int main() {
    int nums[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int index = 5;
    int size = 8;
    int idx = index;
    while (idx < size-1 ) {
        nums[idx] = nums[idx + 1];
        idx++;
    }
    for (int i = 0; i < size; ++i) {
        printf("%d\n", nums[i]);
    }
    return 0;
}