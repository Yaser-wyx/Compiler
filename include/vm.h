//
// Created by wanyu on 2019/10/29.
//

#ifndef _VM_VM_H
#define _VM_VM_H

#include "common.h"

struct vm {
    uint32_t allocatedBytes;
    Parser *curParser;
};

void initVM(VM *);

VM *newVM(void);

#endif //_VM_VM_H
