#pragma once

#include <vector>
#include <string>

namespace lss
{
    namespace base
    {
        using std::string;

        class StringUtils
        {
        public:
            // 字符串前缀匹配
            static bool StartsWith(const string& s, const string& sub);
            
            // 字符串后缀匹配
            static bool EndsWith(const string& s, const string& sub);

            // 返回文件路径，生成不同名的文件
            static std::string FilePath(const std::string& path);

            // 从完整的路径中取出文件名和文件后缀
            static std::string FileNameExt(const std::string& path);

            // 从完整的文件路径中返回文件名
            static std::string FileName(const std::string& path);

            // 返回文件后缀
            static std::string Extension(const std::string& path);

            // 字符串分割
            static std::vector<std::string> SplitString(const string& s, const string& delimiter);

            // 有限状态机的字符串分割
            static std::vector<std::string> SplitStringWithFSM(const string& s, const char delimiter);
        };
    }
}