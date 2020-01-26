//
// Created by yaser on 1/26/2020.
//

#ifndef TEMP_OBJ_THREAD_H
#define TEMP_OBJ_THREAD_H

#include "obj_fn.h"

typedef struct objThread {
    ObjHeader objHeader;
    Value *stack;//bottom of running stack
    Value *esp;//top of running stack
    uint32_t stackCapacity;//运行时栈大小

    Frame *frames;//运行时栈数组
    uint32_t usedFrameNum;//已使用的运行时栈数量
    __uint32_t frameCapacity;//运行时栈容量
    //"打开的upvalue"的链表首结点，包含closedUpvalue与openUpvalue
    ObjUpvalue *openUpvalues;
    //线程调用者
    struct objThread *caller;
    //运行时错误对象
    Value errorObj;
} ObjThread;
void prepareFrame(ObjThread* objThread, ObjClosure* objClosure, Value* stackStart);
ObjThread* newObjThread(VM* vm, ObjClosure* objClosure);
void resetThread(ObjThread* objThread, ObjClosure*  objClosure);
#endif //TEMP_OBJ_THREAD_H
