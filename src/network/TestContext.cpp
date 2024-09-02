#include <iostream>
#include "TestContext.h"

using namespace lss::network;

TestContext::TestContext(const TcpConnectionPtr &con)
    : connection_(con)      // 使用初始化列表初始化连接对象
{

}

int TestContext::ParseMessage(MsgBuffer & buff)
{
    // 当缓冲区中还有可读数据时，持续解析
    while (buff.ReadableBytes() > 0)
    {
        // 如果当前状态是解析消息头
        if (state_ == kTestContextHeader)
        {
            // 如果可读字节数大于等于4，说明可以读取消息长度
            if (buff.ReadableBytes() >= 4)
            {
                // 从缓冲区中读取一个 32 位整数，作为消息长度
                message_length_ = buff.ReadInt32();
                // 打印消息长度用于调试
                std::cout << "message_length_: " << message_length_ << std::endl;
                // 状态转换为解析消息体
                state_ = kTestContextBody;

                // 跳过当前循环，继续下一轮循环以处理消息体
                continue;
            }
            else 
            {
                // 如果缓冲区中可读字节数不足4字节，返回1，表示解析需要更多数据
                return 1;
            }
        }
        // 如果当前状态是解析消息体
        else if (state_ == kTestContextBody)
        {
            // 如果缓冲区中的数据大于等于消息长度
            if (buff.ReadableBytes() >= message_length_)
            {
                // 临时字符串，用于存储消息体
                std::string tmp;
                // 从缓冲区中取出 message_length_ 长度的消息体数据
                tmp.assign(buff.Peek(), message_length_);
                // 将临时字符串追加到消息体成员变量中
                message_body_.append(tmp);
                // 从缓冲区中移除已经读取的数据
                buff.Retrieve(message_length_);
                // 清空消息长度
                message_length_ = 0;
                
                // 如果回调函数已经被设置
                if (cb_)
                {
                    // 执行回调函数，将连接对象和消息体传递给回调函数
                    cb_(connection_, message_body_);
                    // 清空消息体内容，准备下一条消息
                    message_body_.clear();
                }

                // 状态恢复为解析消息头，准备解析下一条消息
                state_ = kTestContextHeader;
            }
        }
    }

    // 返回 1 表示解析完成，等待更多数据
    return 1;
}

void TestContext::SetTestMessageCallback(const TestMessageCallback &cb)
{
    // 将传入的回调函数赋值给成员变量 cb_
    cb_ = cb;
}

void TestContext::SetTestMessageCallback(TestMessageCallback &&cb)
{
    // 使用 std::move 将传入的回调函数移动赋值给成员变量 cb_
    cb_ = std::move(cb);
}