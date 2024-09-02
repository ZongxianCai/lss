#pragma once
#include <string>
#include <functional>
#include <memory>
#include "network/net/TcpConnection.h"

namespace lss
{
    namespace network
    {
        // 定义别名 TestMessageCallback，为一个函数对象类型，接受两个参数：TcpConnectionPtr 和 std::string
        using TestMessageCallback = std::function<void(const TcpConnectionPtr &, const std::string &)>;

        class TestContext
        {
            // 内部枚举，定义两种状态：kTestContextHeader（值为1）和 kTestContextBody（值为2）
            enum
            {
                kTestContextHeader = 1,     // 表示当前解析的是消息头
                kTestContextBody = 2        // 表示当前解析的是消息体
            };

        public:
            // 构造函数，接受一个 TcpConnectionPtr 类型的连接对象
            TestContext(const TcpConnectionPtr &con);

            // 解析消息，接受一个 MsgBuffer 引用作为参数，返回一个整数表示解析结果
            int ParseMessage(MsgBuffer & buff);

            // 设置消息回调函数，接受一个常量引用类型的 TestMessageCallback
            void SetTestMessageCallback(const TestMessageCallback &cb);

            // 设置消息回调函数，接受一个右值引用类型的 TestMessageCallback
            void SetTestMessageCallback(TestMessageCallback &&cb);

            // 析构函数，使用默认析构函数
            ~TestContext() = default;

        private:
            // 保存连接对象的智能指针
            TcpConnectionPtr connection_;

            // 保存当前解析状态，初始化为 kTestContextHeader（解析消息头）
            int state_{kTestContextHeader};

            // 保存消息长度，初始化为0
            int32_t message_length_{0};

            // 保存消息体字符串
            std::string message_body_;

            // 保存消息回调函数
            TestMessageCallback cb_;
        };
    }
}
