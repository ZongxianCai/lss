#pragma once

namespace lss
{
    namespace base
    {
        // 不可复制和移动的基类，属性为protected，不能被类外部直接访问，但可以被派生类访问
        class NonCopyable
        {
        protected:
            // 默认构造函数
            NonCopyable() {}

            // 析构函数
            ~NonCopyable() {}

            // 拷贝构造函数，被声明为delete，编译器不允许使用这个构造函数，即不允许复制NonCopyable类的对象
            NonCopyable(const NonCopyable&) = delete;
            
            // 赋值运算符重载，被声明为delete，意味着不允许对NonCopyable类的对象进行赋值操作
            NonCopyable &operator=(const NonCopyable) = delete;
        };
    }
} 
