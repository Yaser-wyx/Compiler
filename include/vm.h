//
// Created by wanyu on 2019/10/29.
//

#ifndef COMPLIER_VM_H
#define COMPLIER_VM_H

#include "common.h"

struct vm {
    uint32_t allocatedBytes;
    Parser *curParser;
};

void initVM(VM *);

VM *newVM(void);

#endif //COMPLIER_VM_H
