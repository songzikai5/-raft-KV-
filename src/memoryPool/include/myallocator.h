#pragma once
#include <cstdio>
#include <mutex>
#include<cstring>
using namespace std;

/*
多线程 - 线程安全的问题 
- nginx内存池 一个线程中可以创建一个独立的 nginx 内存池来使用移植 
- SGI STL空间配置器是给容器使用的,该容器可能在多线程中使用 
SGI STL 二级空间配置器内存池源码 模板实现的
核心设计：
  1. 第一级配置器：直接封装malloc/free，内存不足时提供oom回调机制
  2. 第二级配置器：内存池+自由链表，处理≤128字节的小内存分配，减少内存碎片和系统调用
  3. 线程安全：通过mutex保护自由链表的读写操作
*/

// ===================== 第一级空间配置器（malloc封装） =====================
// 模板参数__inst：用于区分不同实例（STL源码兼容设计，此处固定为0）
template <int __inst>
class __malloc_alloc_template {

private:
    // 内存不足(oom)时的malloc处理函数：循环调用回调函数直到分配成功
    static void* _S_oom_malloc(size_t);
    // 内存不足(oom)时的realloc处理函数：逻辑同_S_oom_malloc
    static void* _S_oom_realloc(void*, size_t);
    // 内存不足时的回调函数指针（用户可自定义释放内存的逻辑）
    static void (* __malloc_alloc_oom_handler)();

public:
    // 分配内存：直接调用malloc，失败则进入oom处理流程
    static void* allocate(size_t __n)
    {
        void* __result = malloc(__n);
        // malloc失败时调用oom处理函数
        if (0 == __result) __result = _S_oom_malloc(__n);
        return __result;
    }

    // 释放内存：直接调用free（第一级配置器不做内存池管理）
    static void deallocate(void* __p, size_t /* __n */)
    {
        free(__p);
    }

    // 重新分配内存：调用realloc，失败则进入oom处理流程
    static void* reallocate(void* __p, size_t /* old_sz */, size_t __new_sz)
    {
        void* __result = realloc(__p, __new_sz);
        // realloc失败时调用oom处理函数
        if (0 == __result) __result = _S_oom_realloc(__p, __new_sz);
        return __result;
    }

    // 设置内存不足时的回调函数，返回旧的回调函数指针
    static void (* __set_malloc_handler(void (*__f)()))()
    {
        void (* __old)() = __malloc_alloc_oom_handler;
        __malloc_alloc_oom_handler = __f;
        return(__old);
    }
};

// 内存不足时的malloc处理逻辑：循环调用回调函数，直到malloc成功或无回调函数退出
template <int __inst>
void*
__malloc_alloc_template<__inst>::_S_oom_malloc(size_t __n)
{
    void (* __my_malloc_handler)();
    void* __result;

    // 死循环：直到malloc成功或无回调函数（退出程序）
    for (;;) {
        __my_malloc_handler = __malloc_alloc_oom_handler;
        // 无回调函数则打印错误并退出（内存不足）
        if (0 == __my_malloc_handler) { fprintf(stderr, 
            "out of memory\n"); exit(1); }
        // 调用用户自定义的内存释放函数
        (*__my_malloc_handler)();
        // 再次尝试malloc
        __result = malloc(__n);
        if (__result) return(__result);
    }
}

// 内存不足时的realloc处理逻辑：同_S_oom_malloc
template <int __inst>
void* __malloc_alloc_template<__inst>::_S_oom_realloc(void* __p, size_t __n)
{
    void (* __my_malloc_handler)();
    void* __result;

    // 死循环：直到realloc成功或无回调函数（退出程序）
    for (;;) {
        __my_malloc_handler = __malloc_alloc_oom_handler;
        // 无回调函数则打印错误并退出（内存不足）
        if (0 == __my_malloc_handler) { fprintf(stderr, 
            "out of memory\n"); exit(1); }
        // 调用用户自定义的内存释放函数
        (*__my_malloc_handler)();
        // 再次尝试realloc
        __result = realloc(__p, __n);
        if (__result) return(__result);
    }
}

// 初始化oom回调函数指针为nullptr（无默认回调）
template <int inst>
void (*__malloc_alloc_template<inst>::__malloc_alloc_oom_handler)() = nullptr; 

// 定义第一级配置器的别名（模板参数固定为0）
typedef __malloc_alloc_template<0> malloc_alloc;

// ===================== 第二级空间配置器（内存池+自由链表） =====================
template<class T>
class myallocator{

public:
    // STL分配器标准类型别名：值类型
    using value_type =T;

    // 默认构造函数（noexcept：保证不抛出异常）
    constexpr myallocator() noexcept
    {    // construct default allocator (do nothing)
    }

    // 拷贝构造函数（默认实现）
    constexpr myallocator(const myallocator&) noexcept = default;

    // 泛化拷贝构造函数：支持不同类型分配器的转换（空实现）
    template<class _Other>
    constexpr myallocator(const myallocator<_Other>&) noexcept
    {    // construct from a related allocator (do nothing)
    }

    // 开辟内存（核心接口）
    T* allocate(size_t __n);

    // 释放内存（核心接口）
    void deallocate(void* __p,size_t __n);

    // 内存扩容&缩容（核心接口）
    void* reallocate(void* __p,size_t __old_sz,size_t __new_sz);

    // 对象构造：placement new（在已分配内存上构造对象）
    void construct(T*__p,const T &val){
        new (__p) T(val);
    }

    // 对象析构：显式调用析构函数（不释放内存）
    void destroy(T*__p){
        __p->~T();
    }

private:
    enum {_ALIGN = 8};// 自由链表内存块对齐粒度：8字节
    enum {_MAX_BYTES = 128};// 内存池管理的最大内存块大小（超过则用第一级配置器）
    enum {_NFREELISTS = 16};// 自由链表数量：128/8=16（对应8~128字节的内存块）

    // 自由链表节点结构（联合体：节省内存，复用空间）
    union _Obj {
        union _Obj* _M_free_list_link; // 指向下一个空闲chunk块的指针
        char _M_client_data[1];        // 客户端可见的内存起始位置（占位符）
    };

    // 内存池全局状态变量（静态：所有allocator实例共享）
    static char* _S_start_free;    // 内存池起始地址
    static char* _S_end_free;      // 内存池结束地址
    static size_t _S_heap_size;    // 已从堆上分配的总内存大小

    // 16个自由链表（volatile：防止编译器优化，保证多线程可见性）
    static _Obj* volatile _S_free_list[_NFREELISTS];  

    // 互斥锁：保护自由链表的读写操作，保证线程安全
    static mutex mtx;

    // 内存大小向上取整到8的倍数（保证对齐）
    static size_t _S_round_up(size_t __bytes) {
        return (((__bytes) + (size_t)_ALIGN - 1) & ~((size_t)_ALIGN - 1));
    }

    // 根据内存大小计算对应的自由链表下标
    static size_t _S_freelist_index(size_t __bytes) {
        return (((__bytes) + (size_t)_ALIGN - 1) / (size_t)_ALIGN - 1);
    }

    // 填充自由链表：从内存池分配新的chunk块并链接成链表
    static void* _S_refill(size_t __n)
    {
        int __nobjs = 20; // 默认每次分配20个chunk块
        // 从内存池分配__nobjs个大小为__n的chunk块
        char* __chunk = (char*)_S_chunk_alloc(__n, __nobjs);
        _Obj* volatile* __my_free_list; // 目标自由链表指针
        _Obj* __result;                 // 返回的第一个chunk块
        _Obj* __current_obj;            // 当前遍历的chunk块
        _Obj* __next_obj;               // 下一个chunk块
        int __i;

        // 只分配到1个chunk块，直接返回（无需链接链表）
        if (1 == __nobjs) return(__chunk);
        // 找到对应的自由链表
        __my_free_list = _S_free_list + _S_freelist_index(__n);

        /* 把分配到的chunk块链接成自由链表 */
        __result = (_Obj*)__chunk; // 第一个chunk块作为返回值
        // 自由链表头指向第二个chunk块
        *__my_free_list = __next_obj = (_Obj*)(__chunk + __n);
        // 循环链接剩余的chunk块
        for (__i = 1; ; __i++) {
            __current_obj = __next_obj;
            __next_obj = (_Obj*)((char*)__next_obj + __n);
            // 最后一个chunk块，链表尾置空
            if (__nobjs - 1 == __i) {
                __current_obj -> _M_free_list_link = 0;
                break;
            } else {
                // 指向下一个chunk块
                __current_obj -> _M_free_list_link = __next_obj;
            }
        }
        return(__result);
    }

    // 内存池核心：分配指定大小和数量的chunk块（内部调用）
    static char* _S_chunk_alloc(size_t __size, int& __nobjs)
    {
        char* __result;                // 分配结果指针
        size_t __total_bytes = __size * __nobjs; // 需要分配的总字节数
        size_t __bytes_left = _S_end_free - _S_start_free; // 内存池剩余字节数

        // 情况1：内存池剩余空间足够分配__nobjs个chunk块
        if (__bytes_left >= __total_bytes) {
            __result = _S_start_free;
            _S_start_free += __total_bytes; // 移动内存池起始指针
            return(__result); 
        }
        // 情况2：内存池剩余空间不足__nobjs个，但至少能分配1个chunk块
        else if (__bytes_left >= __size) {
            __nobjs = (int)(__bytes_left/__size); // 调整实际分配的数量
            __total_bytes = __size * __nobjs;
            __result = _S_start_free;
            _S_start_free += __total_bytes; // 移动内存池起始指针
            return(__result);
        }
        // 情况3：内存池剩余空间连1个chunk块都不够
        else {
            // 计算需要从堆上分配的新内存大小（2倍需求 + 堆内存增长补偿）
            size_t __bytes_to_get = 2 * __total_bytes + _S_round_up(_S_heap_size >> 4);
            // 利用内存池剩余的小碎片（链接到对应自由链表）
            if (__bytes_left > 0) {
                // 找到剩余碎片对应的自由链表
                _Obj* volatile* __my_free_list =
                            _S_free_list + _S_freelist_index(__bytes_left);
                // 将剩余碎片链接到自由链表头部
                ((_Obj*)_S_start_free) -> _M_free_list_link = *__my_free_list;
                *__my_free_list = (_Obj*)_S_start_free;
            }
            // 从堆上分配新的内存块补充内存池
            _S_start_free = (char*)malloc(__bytes_to_get);
            // malloc失败：尝试从已有的自由链表中借用内存
            if (nullptr == _S_start_free) {
                size_t __i;
                _Obj* volatile* __my_free_list;
                _Obj* __p;
                
                // 遍历所有更大的自由链表，寻找可用内存
                for (__i = __size;
                    __i <= (size_t) _MAX_BYTES;
                    __i += (size_t) _ALIGN) {
                    __my_free_list = _S_free_list + _S_freelist_index(__i);
                    __p = *__my_free_list;
                    // 找到可用的chunk块
                    if (0 != __p) {
                        // 从自由链表中移除该chunk块
                        *__my_free_list = __p -> _M_free_list_link;
                        // 用该chunk块补充内存池
                        _S_start_free = (char*)__p;
                        _S_end_free = _S_start_free + __i;
                        // 递归调用，重新尝试分配
                        return(_S_chunk_alloc(__size, __nobjs));
                    }
                }
                // 所有自由链表都无可用内存：调用第一级配置器
                _S_end_free = 0;	
                _S_start_free = (char*)malloc_alloc::allocate(__bytes_to_get);
            }
            // 更新内存池状态
            _S_heap_size += __bytes_to_get;
            _S_end_free = _S_start_free + __bytes_to_get;
            // 递归调用，重新尝试分配
            return(_S_chunk_alloc(__size, __nobjs));
        }
    }
};

// 静态成员变量初始化（所有allocator实例共享）
template <class T>
char* myallocator<T>::_S_start_free = nullptr; // 内存池起始地址初始化为空

template <class T>
char* myallocator<T>::_S_end_free = nullptr;   // 内存池结束地址初始化为空

template <class T>
size_t myallocator<T>::_S_heap_size = 0;       // 堆内存大小初始化为0

// 16个自由链表初始化为空
template <class T>
typename myallocator<T>::_Obj* volatile myallocator<T>::_S_free_list[_NFREELISTS] = {
    nullptr,nullptr,nullptr,nullptr,
    nullptr,nullptr,nullptr,nullptr,
    nullptr,nullptr,nullptr,nullptr,
    nullptr,nullptr,nullptr,nullptr
};

// 互斥锁静态初始化
template <class T>
mutex myallocator<T>::mtx;

// 开辟内存（对外接口）
template<class T>
T* myallocator<T>::allocate(size_t __n){
    
    void* __ret = 0;

    // 大内存分配：超过128字节，使用第一级配置器
    if (__n > (size_t) _MAX_BYTES) {
      __ret = malloc_alloc::allocate(__n);
    }
    // 小内存分配：使用内存池+自由链表
    else {
        // 找到对应的自由链表
        _Obj* volatile* __my_free_list
          = _S_free_list + _S_freelist_index(__n);

        // 加锁：保证自由链表操作的线程安全（lock_guard自动释放锁）
        lock_guard<mutex>guard(mtx);

        // 从自由链表头部取一个chunk块
        _Obj*  __result = *__my_free_list;
        // 自由链表为空：填充自由链表
        if (__result == 0)
            __ret = _S_refill(_S_round_up(__n));
        // 自由链表有可用chunk块：取下第一个并更新链表头
        else {
            *__my_free_list = __result -> _M_free_list_link;
            __ret = __result;
        }
    }

    return (T*)__ret;
}

// 释放内存（对外接口）
template<class T>
void myallocator<T>::deallocate(void* __p,size_t __n){
    // 大内存释放：使用第一级配置器（直接free）
    if (__n > (size_t) _MAX_BYTES)
        malloc_alloc::deallocate(__p, __n);
    // 小内存释放：归还到对应自由链表
    else {
        // 找到对应的自由链表
        _Obj* volatile*  __my_free_list
            = _S_free_list + _S_freelist_index(__n);
        _Obj* __q = (_Obj*)__p;

        // 加锁：保证自由链表操作的线程安全
        lock_guard<mutex>guard(mtx);
        // 将chunk块插入到自由链表头部（头插法）
        __q -> _M_free_list_link = *__my_free_list;
        *__my_free_list = __q;
    }
}

// 内存扩容&缩容（对外接口）
template<class T>
void* myallocator<T>::reallocate(void* __p,size_t __old_sz,size_t __new_sz){
    void* __result;
    size_t __copy_sz;

    // 新旧内存都大于128字节：直接调用realloc
    if (__old_sz > (size_t) _MAX_BYTES && __new_sz > (size_t) _MAX_BYTES) {
        return(realloc(__p, __new_sz));
    }
    // 新旧内存对齐后大小相同：无需扩容，直接返回原指针
    if (_S_round_up(__old_sz) == _S_round_up(__new_sz)) return(__p);
    // 其他情况：分配新内存 + 拷贝数据 + 释放旧内存
    __result = allocate(__new_sz);
    // 拷贝的字节数：取新旧内存的较小值（防止越界）
    __copy_sz = __new_sz > __old_sz? __old_sz : __new_sz;
    memcpy(__result, __p, __copy_sz);
    // 释放旧内存
    deallocate(__p, __old_sz);
    return(__result);
}