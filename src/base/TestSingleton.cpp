#include <iostream>
#include "Singleton.h"

using namespace lss::base;

// 定义 TestSingleton 类，继承自 NonCopyable 类， TestSingleton 类的对象不能被复制或赋值
class TestSingleton : public NonCopyable
{
public:
    // 使用默认构造函数
    TestSingleton() = default;

    // 使用默认析构函数
    ~TestSingleton() = default;

    void Print()
    {
        std::cout << "This is TestSingleton class !!!" << std::endl;
    }
};

// 定义宏 testSingleton，展开为调用 Singleton 模板类的 Instance 函数，获取 TestSingleton 类的单例对象的引用
#define testSingleton lss::base::Singleton<TestSingleton>::Instance()

int main(int argc, const char ** argv)
{
    // 使用类型自动推断获取单例对象引用的另一种方式
    // auto testSingleton = lss::base::Singleton<TestSingleton>::Instance();
    
    // 通过宏 testSingleton 获取的单例对象引用调用 Print 函数，输出消息
    testSingleton->Print();
    
    return 0;
}
