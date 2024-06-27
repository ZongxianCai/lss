#pragma once
#include <pthread.h>
#include "NonCopyable.h"

namespace lss
{
    namespace base
    {
        template<class T>
        class Singleton : public NonCopyable
        {
        public:
            // 显式删除默认构造函数，防止创建 Singleton 类的实例
            Singleton() = delete;
            // 显式删除析构函数，因为单例对象的生命周期与程序相同，不需要手动析构
            ~Singleton() = delete;

            // 全局访问点函数（静态成员函数），用于获取单例对象的引用
            // 模板类，可以直接在头文件中实现函数的定义
            // T*&，函数返回一个类型为 T*（即类型 T 的指针）的引用， Instance 函数返回的不是一个指针，而是对指针本身的引用。作用：允许调用者通过返回的引用来修改原始指针；确保了对单例实例的引用在函数调用之间保持有效，即使单例对象的生命周期结束
            static T*& Instance()
            {
                // 使用 pthread_once 函数确保初始化函数 Init 只被调用一次，无论 Instance 被调用多少次（通过检查 pthread_once_t 类型的静态成员 ponce_ 来实现）
                // 如果 Init 函数尚未被调用，pthread_once 将调用 Init 函数初始化单例对象
                pthread_once(&ponce_, &Singleton::Init);
                return value_;
            }
        private:
            // 私有静态成员函数，用于初始化单例对象，可以不依赖于类的实例而直接通过类名调用
            // 它首先检查 value_ 是否已初始化，如果没有，则创建一个新的 T 类型的实例
            // 模板类，可以直接在头文件中实现函数的定义
            static void Init()
            {
                if (!value_)
                {
                    // T 是 Singleton 模板类的类型参数，表示单例对象的实际类型
                    value_ = new T();
                }
            }
            // 静态成员变量，用于控制初始化函数 Init 的执行
            static pthread_once_t ponce_;
            // 静态成员变量，用于存储单例对象的指针
            static T *value_;
        };

        // 静态成员变量的初始化需要放在类的外面
        // 模板类静态成员变量 ponce_ 的初始化，确保线程安全
        template<class T>
        pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

        // 模板类静态成员变量 value_ 的初始化，初始值为 nullptr
        template<class T>
        T *Singleton<T>::value_ = nullptr;
    }
}