#include "User.h"
#include "base/TTime.h"
#include "Stream.h"

using namespace lss::live;

User::User(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
    : connection_(ptr)              // 初始化连接指针
    , stream_(stream)               // 初始化流指针
    , session_(s)                   // 初始化会话指针
{
    // 获取当前时间戳并设置为开始时间戳
    start_timestamp_ = lss::base::TTime::NowMS();

    // 从连接指针获取对等地址并设置用户 ID
    user_id_ = ptr->PeerAddr().ToIpPort();
}

const string &User::DomainName() const 
{
    // 返回域名
    return domain_name_;
}

void User::SetDomainName(const string &domain)
{
    // 将传入的域名赋值给成员变量
    domain_name_ = domain;
}

const string &User::AppName() const
{
    // 返回应用名称
    return app_name_;
}

void User::SetAppName(const string &domain)
{
    // 将传入的应用名称赋值给成员变量
    app_name_ = domain;
}

const string &User::StreamName() const
{
    // 返回流名称
    return stream_name_;
}

void User::SetStreamName(const string &domain)
{
    // 将传入的流名称赋值给成员变量
    stream_name_ = domain;
}

const string &User::Param() const
{
    // 返回参数
    return param_;
}

void User::SetParam(const string &domain)
{
    // 将传入的参数赋值给成员变量
    param_ = domain;
}

const AppInfoPtr &User::GetAppInfo()const
{
    // 返回应用信息指针
    return app_info_;
}

void User::SetAppInfo(const AppInfoPtr &info)
{
    // 将传入的应用信息指针赋值给成员变量
    app_info_ = info;
}

UserType User::GetUserType() const
{
    // 返回用户类型
    return type_;
}

void User::SetUserType(UserType t)
{
    // 将传入的用户类型赋值给成员变量
    type_ = t;
}

UserProtocol User::GetUserProtocol() const
{
    // 返回用户协议
    return protocol_;
}

void User::SetUserProtocol(UserProtocol p)
{
    // 将传入的用户协议赋值给成员变量
    protocol_ = p;
}

void User::Close()
{
    // 检查连接指针是否有效
    if (connection_)
    {
        // 强制关闭连接
        connection_->ForceClose();
    }
}

ConnectionPtr User::GetConnection()
{
    // 返回连接指针
    return connection_;
}

uint64_t User::ElapsedTime()
{
    // 返回当前时间戳与开始时间戳的差值
    return lss::base::TTime::NowMS() - start_timestamp_;
}

void User::Active()
{
    // 检查连接指针是否有效
    if (connection_)
    {
        // 激活连接
        connection_->Active();
    }
}

void User::Deactive()
{
    // 检查连接指针是否有效
    if (connection_)
    {
        // 注销连接
        connection_->Deactive();
    }
}