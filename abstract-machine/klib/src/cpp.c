#include <am.h>
#include <klib.h>
#include <klib-macros.h>  // 添加这个头文件，里面定义了panic

#ifndef __ISA_NATIVE__

// 用于处理动态库的句柄
void __dso_handle() {
  // 这个函数可以为空，只需要存在
}

// 用于C++局部静态变量的线程安全初始化
void __cxa_guard_acquire() {
  // 在单线程环境中可以为空
}

void __cxa_guard_release() {
  // 在单线程环境中可以为空
}

// 用于注册析构函数，在程序退出时调用
void __cxa_atexit() {
  // 简单实现，不报错
}

// 用于纯虚函数调用
void __cxa_pure_virtual() {
  panic("Pure virtual function called");
}

// 用于数组去分配
void __cxa_vec_dtor() {
  // 可以为空
}

// 用于异常处理（如果没有启用异常，可以为空）
void __cxa_begin_catch() {
}

void __cxa_end_catch() {
}

void __cxa_rethrow() {
  panic("Exception rethrow not supported");
}

// 注意：以下这些是C++特有的操作符重载
// 在C文件中不能直接实现，需要用编译器特定的方式
// 或者将这些代码放到一个.cpp文件中

#endif

// 如果需要支持C++的new/delete，应该创建一个单独的cpp.cpp文件
// 或者在Makefile中特殊处理