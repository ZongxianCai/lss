#include "FileLog.h"

using namespace lss::base;

// 实现打开日志文件，参数为文件路径
bool FileLog::Open(const std::string &filePath)
{
    file_path_ = filePath;
    // 使用全局命名空间中的 open 函数尝试打开文件
    // 参数 O_CREAT|O_APPEND|O_WRONLY 表示以只写方式打开文件，如果文件不存在则进行创建，并且每次写入时都追加到文件末尾
    // 参数 DEFFILEMODE 是一个宏，定义了文件的默认权限模式
    int fd = ::open(file_path_.c_str(), O_CREAT|O_APPEND|O_WRONLY, DEFFILEMODE);
    
    // 打开失败，输出错误信息并返回 false
    if (fd < 0)
    {
        std::cout << "Open file log error, path: " << filePath << std::endl;

        return false;
    }

    // 更新文件描述符
    fd_ = fd;

    return true;
}

// 实现写入日志消息，参数为要写入的消息字符串
size_t FileLog::WriteLog(const std::string &msg)
{
    // 检查文件描述符 fd 是否有效，无效使用 1 作为文件描述符
    int fd = ((fd_ == -1) ? 1 : fd_);

    // 使用 ::write 函数将消息写入到文件描述符指向的文件
    return ::write(fd, msg.data(), msg.size());
}

// 实现执行日志文件的切分操作，参数为文件名
void FileLog::Rotate(const std::string &file)
{
    // 检查 file_path_ 是否为空，如果为空则直接返回
    if (file_path_.empty())
    {
        return;
    }

    int ret = ::rename(file_path_.c_str(), file.c_str());
    
    if (ret != 0)
    {
        std::cerr << "Rename failed. Old file_name: " << file_path_ << ", New file_name: " << file;
        
        return;
    }

    // 重新打开文件以获取新的文件描述符 fd
    int fd = ::open(file_path_.c_str(), O_CREAT|O_APPEND|O_WRONLY, DEFFILEMODE);
    
    // 打开失败，输出错误信息并返回 false
    if (fd < 0)
    {
        std::cout << "Open file log error, path: " << file << std::endl;

        return;
    }

    // 使用 ::dup2 将新的文件描述符复制到 fd_
    ::dup2(fd, fd_);

    // 关闭 fd
    close(fd);
}

// 实现设置日志文件的文件类型
void FileLog::SetRotate(const RotateType type)
{
    // 设置 rotate_type_ 成员变量
    rotate_type_ = type;
}

// 实现获取当前设置的日志文件切分类型，该操作只是读取值，不会对 FileLog 对象的状态产生任何影响，将其声明为const，可以在 const 限定的对象上被调用
RotateType FileLog::GetRotateType() const
{
    // 返回当前的切分类型
    return rotate_type_;
}

// 实现获取日志文件的大小，该操作只是读取值，不会对 FileLog 对象的状态产生任何影响，将其声明为const，可以在 const 限定的对象上被调用
int64_t FileLog::FileSize() const
{
    // 获取文件大小， lseek64 是 lseek 的64位版本，可以在大文件上使用
    return ::lseek64(fd_, 0, SEEK_END);
}

// 实现获取文件路径
std::string FileLog::FilePath() const
{
    // 返回 file_path_ 成员变量的值
    return file_path_;
}