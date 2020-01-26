//
// Created by yaser on 1/26/2020.
//
#include "obj_thread.h"
#include "vm.h"

/**
 *
 * @param objThread 运行中的线程
 * @param objClosure 运行线程中的闭包函数
 * @param stackStart 运行时栈的起始地址
 */
void prepareFrame(ObjThread *objThread, ObjClosure *objClosure, Value *stackStart) {
    ASSERT(objThread->frameCapacity > objThread->usedFrameNum, "frame is not enough!");
    int frameIndex = objThread->usedFrameNum++;//获取已使用的栈帧数量,usedFrameNum始终指向第一个可用的栈帧
    Frame *frame = &(objThread->frames[frameIndex]);
    frame->closure = objClosure;
    frame->stackStart = stackStart;//初始化frame的起始地址
    frame->ip = objClosure->fn->instrStream.datas;//获取栈帧指令的起始地址，指令的起始地址为闭包函数指令流的起始地址
}

//新建线程，线程中运行的是闭包中的函数。
ObjThread *newObjThread(VM *vm, ObjClosure *objClosure) {
    ASSERT(objClosure != null, "objClosure is null!");
    //初始化线程栈
    Frame *frame = ALLOCATE_ARRAY(vm, Frame, INITIAL_FRAME_NUM);

    //计算栈容量，获取闭包函数所需最大栈空间+1的2次幂的向上取整。
    //+1是为了存储消息的接受者
    uint32_t stackCapacity = ceilToPowerOf2(objClosure->fn->maxStackSlotUsedNum + 1);
    //为栈空间分配内存
    Value *newStack = ALLOCATE_ARRAY(vm, Value, stackCapacity);
    ObjThread *objThread = ALLOCATE(vm, ObjThread);
    initObjHeader(vm, &objThread->objHeader, OT_THREAD, vm->threadClass);
    objThread->stack = newStack;
    objThread->frames = frame;
    objThread->frameCapacity = INITIAL_FRAME_NUM;
    objThread->stackCapacity = stackCapacity;
    resetThread(objThread, objClosure);
    return objThread;
}

//重置线程，用于闭包
void resetThread(ObjThread *objThread, ObjClosure *objClosure) {
    objThread->esp = objThread->stack;//重置栈，将栈顶重置为栈底
    objThread->usedFrameNum = 0;
    objThread->openUpvalues = null;
    objThread->caller = null;
    objThread->errorObj = VT_TO_VALUE(VT_NULL);

    ASSERT(objClosure != null, "objClosure is null in function resetThread!");
    prepareFrame(objThread, objClosure, objThread->stack);
}