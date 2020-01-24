//
// Created by yaser on 1/24/2020.
//
#include "obj_list.h"

//create a list object
ObjList *newObjList(VM *vm, uint32_t elementNum) {
    Value *elementArray = null;
    if (elementNum > 0) {
        //allocate memory for list
        elementArray = ALLOCATE_ARRAY(vm, Value, elementNum);
    }
    //create and init list object
    ObjList *objList = ALLOCATE(vm, ObjList);
    objList->elements.datas = elementArray;
    objList->elements.count = objList->elements.capacity = elementNum;
    initObjHeader(vm, &objList->objHeader, OT_LIST, vm->listClass);
    return objList;
}

void insertElement(VM *vm, ObjList *objList, uint32_t index, Value value) {
    if (index >= objList->elements.count) {
        RUN_ERROR("index is out of boundary!");
    }
    //TODO 存疑，等完成后，需要追踪list的变化情况，可能出现插入的元素之间有null元素
    ValueBufferAdd(vm, &objList->elements, VT_TO_VALUE(VT_NULL));

    uint32_t idx = objList->elements.count - 1;
    while (idx > index) {
        objList->elements.datas[idx] = objList->elements.datas[idx - 1];
        idx++;
    }
    objList->elements.datas[index] = value;
}

static void shrinkList(VM *vm, ObjList *objList, uint32_t newCapacity) {
    uint32_t oldSize = objList->elements.capacity * sizeof(Value);
    uint32_t newSize = sizeof(Value) * newCapacity;
    memManager(vm, objList->elements.datas, oldSize, newSize);
    objList->elements.capacity = newCapacity;
}

Value removeElement(VM *vm, ObjList *objList, uint32_t index) {
    if (index >= objList->elements.count) {
        RUN_ERROR("element is not exist!");
    }
    Value removedValue = objList->elements.datas[index];

    uint32_t idx = index;
    while (idx < objList->elements.count - 1) {
        objList->elements.datas[idx] = objList->elements.datas[idx + 1];
        idx++;
    }

    uint32_t _capacity = objList->elements.capacity / CAPACITY_GROW_FACTOR;
    if(_capacity > objList->elements.count){
        shrinkList(vm, objList, _capacity);
    }
    objList->elements.count--;
    return removedValue;
}