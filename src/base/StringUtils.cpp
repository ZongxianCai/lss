#include "StringUtils.h"

using namespace lss::base;

// 字符串前缀匹配
bool StringUtils::StartsWith(const string& s, const string& sub)
{
    if (sub.empty())
    {
        return true;
    }

    if (s.empty())
    {
        return false;
    }

    // 原字符串的长度
    auto len = s.size();
    // 子串的长度
    auto sLen = sub.size();

    if (len < sLen)
    {
        return false;
    }

    // s（调用字符串）和sub（被调用字符串）的值相同，返回0；
    // s按字典序先于sub，返回负值；
    // sub按字典序先于s，返回正值。
    // s的下标为0的字符开始的sLen个字符和sub进行比较
    return s.compare(0, sLen, sub) == 0;
}

// 字符串后缀匹配
bool StringUtils::EndsWith(const string& s, const string& sub)
{
    if (sub.empty())
    {
        return true;
    }

    if (s.empty())
    {
        return false;
    }

    auto len = s.size();
    auto sLen = sub.size();

    if (len < sLen)
    {
        return false;
    }

    return s.compare(len - sLen, sLen, sub) == 0;
}

// 返回文件路径，生成不同名的文件
std::string StringUtils::FilePath(const std::string& path)
{
    // 查找文件路径的结束位置(/ or \)， \ 需要转义
    auto pos = path.find_last_of("/\\");

    // string::npos:字符串的结束位置
    if (pos != std::string::npos)
    {
        // 找到了，返回文件路径
        // 复制子串，从位置0开始，复制pos个长度
        return path.substr(0, pos);
    } 
    else
    {
        // 未找到，返回当前路径
        return "./";
    }
}

// 从完整的路径中取出文件名和文件后缀
std::string StringUtils::FileNameExt(const std::string& path)
{
    auto pos = path.find_last_of("/\\");

    if (pos != std::string::npos)
    {
        // 文件路径结束位置的后一个字符
        if (pos + 1 < path.size())
        {
            // 从 pos + 1 开始到文件名最后
            return path.substr(pos + 1); 
        }
    }
    
    // 未找到，返回文件路径
    return path;
}

// 从完整的文件路径中返回文件名
std::string StringUtils::FileName(const std::string& path)
{
    string file_name = FileNameExt(path);
    auto pos = path.find_last_of(".");

    if (pos != std::string::npos)
    {
        if (pos != 0)
        {
            return file_name.substr(0, pos);
        }
    }

    return file_name;
}

// 返回文件后缀
std::string StringUtils::Extension(const std::string& path)
{
    string file_name = FileNameExt(path);
    auto pos = file_name.find_last_of(".");

    if (pos != std::string::npos)
    {
        if (pos != 0 && pos + 1 < file_name.size())
        {
            return file_name.substr(pos + 1);
        }
    }

    return std::string();
}

// 字符串分割，将字符串s按照指定的分隔符delimiter进行分割
std::vector<std::string> StringUtils::SplitString(const string& s, const string& delimiter)
{
    // 空分隔符
    if (delimiter.empty())
    {
        // 返回一个空列表
        return std::vector<std::string>{};
    }

    // result用于存储分割后的子字符串，临时变量定义在返回值的内存空间中，不会产生额外开销
    std::vector<std::string> result;
    // 查找分隔符位置的索引
    size_t last = 0;
    size_t next = 0;
    
    // 循环查找和分割字符串，查找从last位置开始的第一个delimiter出现的位置next
    while ((next = s.find(delimiter, last)) != std::string::npos)
    {
        // 找到且next > last，存在一个从last到next之间的非空的子字符串
        if (next > last)
        {
            // 提取该子串，添加到result中
            // 使用emplace_back()直接在容器末尾构造对象，避免额外的拷贝或移动操作(push_back需要传递一个已构造的对象)
            result.emplace_back(s.substr(last, next - last));
        }
        else
        {
            // 没有找到，仅添加一个空字符串
            result.emplace_back("");
        }
        // 更新last，跳过已经处理的分隔符
        last = next + delimiter.size();
    }

    // 循环结束后，last仍小于s的长度，说名字符串s中从last到末尾还有一个子字符串未处理，将其提取并添加到result中
    if (last < s.size())
    {
        result.emplace_back(s.substr(last));
    }
    
    return result;
}

// 有限状态机的字符串分割，将字符串s按照指定的分隔符delimiter进行分割
std::vector<std::string> StringUtils::SplitStringWithFSM(const string& s, const char delimiter)
{
    // 定义状态机的状态枚举值
    enum
    {
        kStateInit = 0,             // 初始状态
        kStateNormal = 1,           // 正常状态，处理非分隔符字符
        kStateDelimiter = 2,        // 分隔符状态，处理分隔符
        kStateEnd = 3               // 结束状态
    };

    // result 用于存储分割后的子字符串，临时变量定义在返回值的内存空间中，不会产生额外开销
    std::vector<std::string> result;
    
    // 初始化状态为初始状态
    int state = kStateInit;

    // 临时字符串变量，用于存储当前解析的子字符串
    std::string tmp;

    // 将状态设置为正常状态，准备开始解析字符串
    state = kStateNormal;

    // 遍历输入字符串 s
    for (int pos = 0; pos < s.size();)
    {
        // 当前状态为正常状态，处理非分隔符字符
        if (state == kStateNormal)
        {
            // 如果当前字符是分隔符
            if (s.at(pos) == delimiter)
            {
                // 状态转移到分隔符状态
                state = kStateDelimiter;
                // 跳过本次循环，不增加 pos 以便在下次处理
                continue;
            }

            // 如果当前字符不是分隔符，将其添加到临时字符串 tmp 中
            tmp.push_back(s.at(pos));
            // 移动到下一个字符
            pos++;
        }
        // 当前状态为分隔符状态
        else if (state == kStateDelimiter)
        {
            // 将临时字符串 tmp 存入结果向量 result 中
            result.push_back(tmp);
            // 清空临时字符串以存储下一个子字符串
            tmp.clear();
            // 状态转移回正常状态
            state = kStateNormal;
            // 继续处理下一个字符
            pos++;
        }
    }

    // 如果循环结束后临时字符串 tmp 中还有内容，说明最后一个子字符串没有被添加到 result 中
    if (!tmp.empty())
    {
        // 将最后一个子字符串添加到结果向量中
        result.push_back(tmp);
    }

    // 返回分割后的结果向量
    return result;
}