//
// Created by wanyu on 2019/10/29.
//
#include "core.h"
#include <string.h>
#include <sys/stat.h>
#include "utils.h"
#include "vm.h"
#include "meta_obj.h"
#include "compiler.h"
#include "obj_thread.h"

char *rootDir = null;
#define CORE_MODULE VT_TO_VALUE(VT_NULL)
//返回值类型是Value类型,且是放在args[0], args是Value数组
//RET_VALUE的参数就是Value类型,无须转换直接赋值.
//它是后面"RET_其它类型"的基础
#define RET_VALUE(value)\
do{\
    args[0] = value;\
    return true;\
}while(0);

//将obj转换为Value后做为返回值
#define RET_OBJ(objPtr) RET_VALUE(OBJ_TO_VALUE(objPtr))

//将各种值转为Value后做为返回值
#define RET_BOOL(boolean) RET_VALUE(BOOL_TO_VALUE(boolean))
#define RET_NUM(num) RET_VALUE(NUM_TO_VALUE(num))
#define RET_NULL RET_VALUE(VT_TO_VALUE(VT_NULL))
#define RET_TRUE RET_VALUE(VT_TO_VALUE(VT_TRUE))
#define RET_FALSE RET_VALUE(VT_TO_VALUE(VT_FALSE))

//设置线程报错
#define SET_ERROR_FALSE(vmPtr, errMsg)\
    do{\
        vmPtr->curThread->errorObj = OBJ_TO_VALUE(newObjString(vmPtr,errMsg,strlen(errMsg)));\
        returm false;\
    }while(0);

//绑定原生方法func到classPtr指向的类中，用于替换脚本方法
#define PRIM_METHOD_BIND(classPtr, methodName, func){\
   uint32_t length = strlen(methodName);\
   int globalIdx = getIndexFromSymbolTable(&vm->allMethodNames, methodName, length);\
   if (globalIdx == -1) {\
      globalIdx = addSymbol(vm, &vm->allMethodNames, methodName, length);\
   }\
   Method method;\
   method.type = MT_PRIMITIVE;\
   method.primFn = func;\
   bindMethod(vm, classPtr, (uint32_t)globalIdx, method);\
}

//从文件中读取源码
char *readFile(const char *path) {
    FILE *file = fopen(path, "r");//打开文件
    if (isNull(file)) {
        IO_ERROR("can't open the file \"%s\".\n", path);
    }
    struct stat fileStat;
    stat(path, &fileStat);//获取文件属性
    size_t fileSize = fileStat.st_size;
    char *fileContent = (char *) malloc(fileSize + 1);//为文件缓冲区分配内存，其中+1为了最后置0
    if (isNull(fileContent)) {
        MEM_ERROR("can't allocate memory for reading file \"$s\".\n", path);
    }
    size_t numRead = fread(fileContent, sizeof(char), fileSize, file);//从文件中读取数据到文件缓冲区中，并返回读入的数据量
    if (numRead < fileSize) {
        IO_ERROR("can't read file \"%s\".\n", path);
    }
    fileContent[fileSize] = '\0';//将缓冲区中的最后一位置0
    fclose(file);
    return fileContent;
}

// !obj: 对象取反为false
static bool primObjectNot(VM *vm UNUSED, Value *args) {
    RET_FALSE
}

//args[0] == args[1]: 返回object是否相等
static bool primObjectEqual(VM *vm UNUSED, Value *args) {
    RET_VALUE(BOOL_TO_VALUE(valueIsEqual(args[0], args[1])))
}

//args[0] != args[1]: 返回object是否不等
static bool primObjectNotEqual(VM *vm UNUSED, Value *args) {
    RET_VALUE(BOOL_TO_VALUE(!valueIsEqual(args[0], args[1])))
}

//args[0] is args[1]:类args[0]是否为类args[1]的子类
static bool primObjectIs(VM *vm UNUSED, Value *args) {
    if (!VALUE_IS_CLASS(args[1])) {
        //args[1]必须是类
        RUN_ERROR("argument must be class!");
    }
    Class *thisClass = getClassOfObj(vm, args[0]);
    Class *baseClass = (Class *) (args[1].objHeader);//通过objHeader强转为class
    //可能是多级继承，自下而上查找父类
    //TODO 该方法实现与书中不一致，个人认为书中代码有问题，待验证
    while (thisClass != null) {
        if (thisClass == baseClass) {
            RET_TRUE
        }
        thisClass = thisClass->superClass;
    }
    RET_FALSE
}

//args[0].tostring: 返回args[0]所属class的名字
static bool primObjectToString(VM *vm UNUSED, Value *args) {
    Class *class = args[0].objHeader->class;
    Value nameValue = OBJ_TO_VALUE(class->name);
    RET_VALUE(nameValue);
}

//args[0].type:返回对象args[0]的类
static bool primObjectType(VM *vm, Value *args) {
    Class *class = getClassOfObj(vm, args[0]);
    RET_OBJ(class);
}

//args[0].name: 返回类名
static bool primClassName(VM *vm UNUSED, Value *args) {
    RET_OBJ(VALUE_TO_CLASS(args[0])->name);
}

//args[0].supertype: 返回args[0]的基类
static bool primClassSupertype(VM *vm UNUSED, Value *args) {
    Class *class = VALUE_TO_CLASS(args[0]);
    if (class->superClass != NULL) {
        RET_OBJ(class->superClass);
    }
    RET_VALUE(VT_TO_VALUE(VT_NULL));
}

//args[0].toString: 返回类名
static bool primClassToString(VM *vm UNUSED, Value *args) {
    RET_OBJ(VALUE_TO_CLASS(args[0])->name);
}

//args[0].same(args[1], args[2]): 返回args[1]和args[2]是否相等
static bool primObjectMetaSame(VM *vm UNUSED, Value *args) {
    Value boolValue = BOOL_TO_VALUE(valueIsEqual(args[1], args[2]));
    RET_VALUE(boolValue);
}

//根据模块名到虚拟机所有的模块表中，获取指定模块
static ObjModule *getModule(VM *vm, Value moduleName) {
    Value value = mapGet(vm->allModules, moduleName);
    if (value.type == VT_UNDEFINED) {
        //不存在
        return null;
    }
    return VALUE_TO_OBJMODULE(value);//将value结构转化为module结构
}

static ObjThread *loadModule(VM *vm, Value moduleName, const char *moduleCode) {
    ObjModule *module = getModule(vm, moduleName);
    if (module == null) {
        //如果为null，则进行导入
        ObjString *modString = VALUE_TO_OBJSTR(moduleName);
        ASSERT(modString->value.start[modString->value.length] == '\0', "string.value.start is not terminated!");
        module = newObjModule(vm, modString->value.start);
        ObjModule *coreModule = VALUE_TO_OBJMODULE(mapGet(vm->allModules, CORE_MODULE));
        for (int index = 0; index < coreModule->moduleVarName.count; ++index) {
            defineModuleVar(vm, module, coreModule->moduleVarName.datas[index].str,
                            strlen(coreModule->moduleVarName.datas[index].str),
                            coreModule->moduleVarValue.datas[index]);
        }
    }
    //将模块编译为方法指令流
    ObjFn *fn = compileModule(vm, module, moduleCode);
    //将方法指令流封装为闭包对象
    ObjClosure *closure = newObjClosure(vm, fn);
    //将创建一个新的线程来运行含有新建模块的闭包对象
    ObjThread *moduleThread = newObjThread(vm, closure);
    return moduleThread;
}

VMResult executeModule(VM *vm, Value moduleName, const char *moduleCode) {
    //todo 站桩函数，目前为空
    return VM_RESULT_ERROR;
}

//table中查找符号symbol 找到后返回索引,否则返回-1
int getIndexFromSymbolTable(SymbolTable *table, const char *symbol, uint32_t length) {
    ASSERT(length != 0, "length of symbol is 0!");
    for (int index = 0; index < table->count; ++index) {
        if (table->datas[index].length == length && memcmp(table->datas[index].str, symbol, length) == 0) {
            return index;
        }
    }
    return -1;
}

//往table中添加符号symbol,返回其索引
int addSymbol(VM *vm, SymbolTable *table, const char *symbol, uint32_t length) {
    ASSERT(length != 0, "length of symbol is 0!");
    String string;
    string.str = ALLOCATE_ARRAY(vm, char, length + 1);
    string.length = length;
    memcpy(string.str, symbol, length);
    string.str[length] = '\0';
    StringBufferAdd(vm, table, string);
    return table->count - 1;
}

static Class *defineClass(VM *vm, ObjModule *objModule, const char *name) {
    Class *class = newRawClass(vm, name, 0);//新建一个裸类
    defineModuleVar(vm, objModule, name, strlen(name), OBJ_TO_VALUE(class));//将类作为模块变量保存在模块中
    return class;
}

//绑定方法到类中
void bindMethod(VM *vm, Class *class, __uint32_t index, Method method) {
    if (index >= class->methods.count) {
        Method empty = {MT_NONE, {0}};
        MethodBufferFillWrite(vm, &class->methods, empty, index - class->methods.count + 1);
    }
    class->methods.datas[index] = method;
}

//绑定基类
void bindSuperClass(VM *vm, Class *subClass, Class *superClass) {
    subClass->superClass = superClass;
    subClass->fieldNum += superClass->fieldNum;
    for (int index = 0; index < subClass->methods.count; ++index) {
        bindMethod(vm, subClass, index, superClass->methods.datas[index]);
    }
}

void buildCore(VM *vm) {
    //编译核心模块
    ObjModule *coreModule = newObjModule(vm, null);//实例化核心模块
    mapSet(vm, vm->allModules, CORE_MODULE, OBJ_TO_VALUE(coreModule));
    //创建object类，并绑定方法
    vm->objectClass = defineClass(vm, coreModule, "object");
    PRIM_METHOD_BIND(vm->objectClass, "!", primObjectNot)
    PRIM_METHOD_BIND(vm->objectClass, "==(_)", primObjectEqual);
    PRIM_METHOD_BIND(vm->objectClass, "!=(_)", primObjectNotEqual);
    PRIM_METHOD_BIND(vm->objectClass, "is(_)", primObjectIs);
    PRIM_METHOD_BIND(vm->objectClass, "toString", primObjectToString);
    PRIM_METHOD_BIND(vm->objectClass, "type", primObjectType);

    vm->classOfClass = defineClass(vm, coreModule, "class");
    bindSuperClass(vm, vm->classOfClass, vm->objectClass);//将class作为object的子类
    //绑定原生方法
    PRIM_METHOD_BIND(vm->classOfClass, "name", primClassName);
    PRIM_METHOD_BIND(vm->classOfClass, "supertype", primClassSupertype);
    PRIM_METHOD_BIND(vm->classOfClass, "toString", primClassToString);

    //定义object类的原信息类
    Class *objectMetaClass = defineClass(vm, coreModule, "objectMeta");

    bindSuperClass(vm, objectMetaClass, vm->classOfClass);//为objectMetaClass绑定父类

    PRIM_METHOD_BIND(objectMetaClass, "same(_,_)", primObjectMetaSame);
    //为各个类设置metaClass
    vm->classOfClass->objHeader->class = vm->classOfClass;
    vm->objectClass->objHeader->class = objectMetaClass;
    objectMetaClass->objHeader->class = vm->classOfClass;
}
