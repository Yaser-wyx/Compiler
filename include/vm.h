//
// Created by wanyu on 2019/10/29.
//

#ifndef _VM_VM_H
#define _VM_VM_H

#include "common.h"
#include "class.h"
#include "parser.h"

struct vm {
    Class *stringClass;
    Class *rangeClass;
    Class *mapClass;
    Class *fnClass;
    Class *listClass;
    ObjHeader *allObjects;//obj list
    uint32_t allocatedBytes;
    Parser *curParser;

};

void initVM(VM *);

VM *newVM(void);

#endif //_VM_VM_H
