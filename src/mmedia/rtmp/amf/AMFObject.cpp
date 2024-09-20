#include "AMFObject.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "AMFBoolean.h"
#include "AMFNumber.h"
#include "AMFDate.h"
#include "AMFString.h"
#include "AMFLongString.h"
#include "AMFNull.h"

using namespace lss::mm;

namespace 
{
    // 定义一个静态变量 any_ptr_null，类型为 AMFAnyPtr
    // 这个变量在整个文件中都可以使用，但在其他文件中不可见
    // 由于它是静态的，它的生命周期从程序开始到结束
    static AMFAnyPtr any_ptr_null;
}

AMFObject::AMFObject(const std::string &name)
:AMFAny(name)               // 初始化 name_ 成员变量
{

}

AMFObject::AMFObject()
{

}

int AMFObject::Decode(const char *data, int size, bool has)
{
    // 用于存储属性名称的字符串
    std::string nname;

    // 用于记录已解析的字节数
    int32_t parsed = 0;

    // 循环解析数据，每次检查剩余数据大小是否足够处理下一个 AMF 类型
    while ((parsed + 3) <= size)
    {
        // 检查是否遇到 AMF 对象结束标志（0x000009）
        if (BytesReader::ReadUint24T(data) == 0x000009)
        {
            // 更新已解析字节数，跳过结束标志的 3 个字节
            parsed += 3;
            // 返回已解析字节数，终止解析
            return parsed;
        }

        // 如果 has 标志为真，表示需要解析属性名称
        if (has)
        {
            // 调用 DecodeString 函数解码属性名称
            nname = DecodeString(data);

            // 如果属性名称不为空，则更新解析指针和已解析字节数
            if (!nname.empty())
            {
                // 更新已解析字节数，跳过名称的长度加上 2 个字节（表示字符串长度的字节数）
                parsed += (nname.size() + 2);

                // 更新数据指针，指向下一个字段
                data += (nname.size() + 2);  
            }          
        }

        // 读取下一个字节，这个字节表示 AMF 数据的类型
        char type = *data++;

        // 已解析字节数加 1
        parsed ++;

        // 根据不同的 AMF 类型进行相应的解析
        switch (type)
        {
            // 处理 AMF Number 类型
            case kAMFNumber:
            {
                // 创建 AMFNumber 对象的智能指针
                std::shared_ptr<AMFNumber> p = std::make_shared<AMFNumber>(nname);

                // 调用 AMFNumber 的 Decode 函数进行解码
                auto len = p->Decode(data, size - parsed);

                // 如果解码失败，返回 -1 终止解析
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 输出解码后的 Number 值
                RTMP_TRACE << " Number value : " << p->Number();

                // 将解析后的 AMFNumber 对象存储到 properties_ 容器中
                properties_.emplace_back(std::move(p));
                break;
            }

            // 处理 AMF Boolean 类型
            case kAMFBoolean:
            {
                // 创建 AMFBoolean 对象的智能指针
                std::shared_ptr<AMFBoolean> p = std::make_shared<AMFBoolean>(nname);

                // 调用 AMFBoolean 的 Decode 函数进行解码
                auto len = p->Decode(data, size - parsed);

                // 如果解码失败，返回 -1 终止解析
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 输出解码后的 Boolean 值
                RTMP_TRACE << " Boolean value : " << p->Number();

                // 将解析后的 AMFBoolean 对象存储到 properties_ 容器中
                properties_.emplace_back(std::move(p));                
                break;
            }

            // 处理 AMF String 类型
            case kAMFString:
            {
                // 创建 AMFString 对象的智能指针
                std::shared_ptr<AMFString> p = std::make_shared<AMFString>(nname);

                // 调用 AMFString 的 Decode 函数进行解码
                auto len = p->Decode(data, size - parsed);

                // 如果解码失败，返回 -1 终止解析
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 输出解码后的 String 值
                RTMP_TRACE << " String value : " << p->String();

                // 将解析后的 AMFString 对象存储到 properties_ 容器中
                properties_.emplace_back(std::move(p));                
                break;                
            }

            // 处理 AMF Object 类型
            case kAMFObject:
            {
                // 创建 AMFObject 对象的智能指针
                std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);

                // 调用 AMFObject 的 Decode 函数进行解码，传递 true 以解析内部对象属性
                auto len = p->Decode(data, size - parsed, true);

                // 如果解码失败，返回 -1 终止解析
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 输出调试信息，表明正在处理对象
                RTMP_TRACE << " Object : ";

                // 输出对象的详细信息
                p->Dump();

                // 将解析后的 AMFObject 对象存储到 properties_ 容器中
                properties_.emplace_back(std::move(p));                
                break;                
            }     

            // 处理 AMF Null 类型
            case kAMFNull:
            {
                // 输出调试信息，表明遇到了 Null 类型
                RTMP_TRACE << " Null.";

                // 将解析后的 AMFObject 对象存储到 properties_ 容器中
                properties_.emplace_back(std::move(std::make_shared<AMFNull>()));  
                break;
            }

            // 处理 AMF EcmaArray 类型
            case kAMFEcmaArray:
            {
                // 读取数组的元素数量
                int count = BytesReader::ReadUint32T(data);

                // 更新已解析字节数，跳过 4 个字节的数量信息
                parsed += 4;

                // 更新数据指针，跳过数量字段
                data += 4;

                // 创建 AMFObject 对象的智能指针，用于存储数组元素
                std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);

                // 调用 AMFObject 的 Decode 函数解码数组的元素
                auto len = p->Decode(data, size - parsed, true);

                // 如果解码失败，返回 -1 终止解析
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 输出调试信息，表明正在处理 EcmaArray
                RTMP_TRACE << " EcmaArray : ";

                // 输出数组的详细信息
                p->Dump();

                // 将解析后的 EcmaArray 对象存储到 properties_ 容器中
                properties_.emplace_back(std::move(p));                
                break;  
            }    

            // 处理 AMF ObjectEnd 类型
            case kAMFObjectEnd:
            {
                // 返回已解析字节数，表示对象解析结束
                return parsed;
            }

            // 处理 AMF StrictArray 类型
            case kAMStrictArray:
            {
                // 读取数组的元素数量
                int count = BytesReader::ReadUint32T(data);

                // 更新已解析字节数，跳过 4 个字节的数量信息
                parsed += 4;

                // 更新数据指针，跳过数量字段
                data += 4;

                // 创建 AMFObject 对象的智能指针，用于存储数组元素
                std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);

                // 循环解码每一个数组元素，直到解析完所有元素
                while (count > 0)
                {
                    // 调用 DecodeOnce 函数单次解码一个元素
                    auto len = p->DecodeOnce(data, size - parsed, true);

                    // 如果解码失败，返回 -1 终止解析
                    if (len == -1)
                    {
                        return -1;
                    }

                    // 更新数据指针，跳过解码的字节
                    data += len;

                    // 更新已解析字节数
                    parsed += len;

                    // 元素数量减 1
                    count --;
                }

                // 输出调试信息，表明正在处理 EcmaArray
                RTMP_TRACE << " EcmaArray : ";

                // 输出数组的详细信息
                p->Dump();

                // 将解析后的 StrictArray 对象存储到 properties_ 容器中
                properties_.emplace_back(std::move(p));    
                break;
            }

            // 处理 AMF Date 类型
            case kAMFDate:
            {
                // 创建一个 AMFDate 对象的智能指针，使用当前的属性名称
                std::shared_ptr<AMFDate> p = std::make_shared<AMFDate>(nname);

                // 解码数据并更新解码的字节数
                auto len = p->Decode(data, size - parsed);

                // 如果解码失败，则返回 -1
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 输出解码后的日期值用于调试
                RTMP_TRACE << " Date value : " << p->Date();

                // 将解码后的 AMFDate 对象添加到属性列表中
                properties_.emplace_back(std::move(p));                
                break;   
            }

            // 处理 AMF 数据类型为长字符串 (LongString) 的情况
            case kAMFLongString:
            {
                // 创建一个 AMFLongString 对象的智能指针，使用当前的属性名称
                std::shared_ptr<AMFLongString> p = std::make_shared<AMFLongString>(nname);

                // 解码数据并更新解码的字节数
                auto len = p->Decode(data, size - parsed);

                // 如果解码失败，则返回 -1
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 输出解码后的长字符串值用于调试
                RTMP_TRACE << " LongString value : " << p->String();

                // 将解码后的 AMFLongString 对象添加到属性列表中
                properties_.emplace_back(std::move(p));                
                break;  
            }

            case kAMFMovieClip:

            case kAMFUndefined:

            case kAMFReference:

            case kAMFUnsupported:

            case kAMFRecordset:

            case kAMFXMLDoc:

            case kAMFTypedObject:

            // 处理其他不支持的 AMF 数据类型
            case kAMFAvmplus:
            {
                // 输出不支持的类型，用于调试
                RTMP_TRACE << " not surpport type : " << type;
                break;
            }
        }
    }

    // 返回解析的字节数
    return parsed;
}

bool AMFObject::IsObject()
{
    // 始终返回 true，因为 AMFObject 类本身就是一个对象类型
    return true;
}

AMFObjectPtr AMFObject::Object()
{
    // 返回当前对象的智能指针，使用 dynamic_pointer_cast 将当前对象的 shared_ptr 转换为 AMFObjectPtr 类型，shared_from_this() 返回当前对象的 shared_ptr
    return std::dynamic_pointer_cast<AMFObject>(shared_from_this());
}

void AMFObject::Dump() const
{
    // 输出 "Object start." 作为调试信息，标识对象的开始
    RTMP_TRACE << " Object start.";

    // 遍历 properties_ 向量中的每个属性
    for (auto const &p : properties_)
    {
        // 调用每个属性的 Dump() 方法，输出其调试信息
        p->Dump();
    }

    // 输出 "Object end." 作为调试信息，标识对象的结束
    RTMP_TRACE << " Object end.";
}

// 单次解析 AMF 数据的属性
int AMFObject::DecodeOnce(const char *data, int size, bool has)
{
    // 用于存储属性名称的字符串
    std::string nname;

    // 已解析的数据字节数
    int32_t parsed = 0;

    // 如果需要解析属性名称
    if (has)
    {
        // 从数据中解码出属性名称
        nname = DecodeString(data);

        // 如果不为空
        if (!nname.empty())
        {
            // 更新已解析的字节数，包括名称的长度和名称长度的2字节前缀
            parsed += (nname.size() + 2);

            // 移动数据指针到属性值的起始位置
            data += (nname.size() + 2);
        }
    }

    // 读取属性类型，并将数据指针移动到下一个字节
    char type = *data++;

    // 更新已解析的字节数（读取了属性类型）
    parsed ++;

    switch (type)
    {
        // 处理 AMF 数字类型
        case kAMFNumber:
        {
            // 创建 AMFNumber 对象并传递属性名称
            std::shared_ptr<AMFNumber> p = std::make_shared<AMFNumber>(nname);

            // 解码数字属性值
            auto len = p->Decode(data, size - parsed);

            // 如果解码失败，返回 -1
            if (len == -1)
            {
                return -1;
            }

            // 更新数据指针，跳过解码的字节
            data += len;

            // 更新已解析字节数
            parsed += len;

            // 输出解码后的数字值
            RTMP_TRACE << " Number value : " << p->Number();

            // 将解码后的数字属性添加到属性列表中
            properties_.emplace_back(std::move(p));

            break;
        }

        // 处理 AMF 布尔类型
        case kAMFBoolean:
        {
            // 创建 AMFBoolean 对象并传递属性名称
            std::shared_ptr<AMFBoolean> p = std::make_shared<AMFBoolean>(nname);

            // 解码布尔属性值
            auto len = p->Decode(data, size - parsed);

            // 如果解码失败，返回 -1
            if (len == -1)
            {
                return -1;
            }

            // 更新数据指针，跳过解码的字节
            data += len;

            // 更新已解析字节数
            parsed += len;

            // 输出解码后的布尔值（布尔值在 AMF 中也可能作为数字显示）
            RTMP_TRACE << " Boolean value : " << p->Number();

            // 将解码后的布尔属性添加到属性列表中
            properties_.emplace_back(std::move(p));

            break;
        }

        // 处理 AMF 字符串类型
        case kAMFString:
        {
            // 创建 AMFString 对象并传递属性名称
            std::shared_ptr<AMFString> p = std::make_shared<AMFString>(nname);

            // 解码字符串属性值
            auto len = p->Decode(data, size - parsed);

            // 如果解码失败，返回 -1
            if (len == -1)
            {
                return -1;
            }

            // 更新数据指针，跳过解码的字节
            data += len;

            // 更新已解析字节数
            parsed += len;

            // 输出解码后的字符串值
            RTMP_TRACE << " String value : " << p->String();

            // 将解码后的字符串属性添加到属性列表中
            properties_.emplace_back(std::move(p));                
            break;                
        }

        // 处理 AMF 对象类型
        case kAMFObject:
        {
            // 创建 AMFObject 对象并传递属性名称
            std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);

            // 解码嵌套的 AMF 对象属性值
            auto len = p->Decode(data, size - parsed, true);

            // 如果解码失败，返回 -1
            if (len == -1)
            {
                return -1;
            }

            // 更新数据指针，跳过解码的字节
            data += len;

            // 更新已解析字节数
            parsed += len;

            // 输出解码后的对象的调试信息
            RTMP_TRACE << " Object : ";

            // 调用每个属性的 Dump() 方法，输出其调试信息
            p->Dump();

            // 将解码后的对象属性添加到属性列表中
            properties_.emplace_back(std::move(p));                
            break;                
        }

        // 处理 AMF Null 类型
        case kAMFNull:
        {
            // 输出 "Null." 调试信息
            RTMP_TRACE << " Null.";

            // 将解析后的 AMFObject 对象存储到 properties_ 容器中
            properties_.emplace_back(std::move(std::make_shared<AMFNull>()));
            break;
        }

        // 处理 AMF EcmaArray 类型（类似于对象的数组）
        case kAMFEcmaArray:
        {
            int count = BytesReader::ReadUint32T(data);
            // 更新已解析字节数，跳过 4 个字节的数量信息
            parsed += 4;

            // 更新数据指针，跳过数量字段
            data += 4;

            // 创建 AMFObject 对象
            std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);

            // 解码 EcmaArray 属性值
            auto len = p->Decode(data, size - parsed, true);

            // 如果解码失败，返回 -1
            if (len == -1)
            {
                return -1;
            }

            // 更新数据指针，跳过解码的字节
            data += len;

            // 更新已解析字节数
            parsed += len;

            // 输出解码后的 EcmaArray 的调试信息
            RTMP_TRACE << " EcmaArray : ";

            // 调用每个属性的 Dump() 方法，输出其调试信息
            p->Dump();

            // 将解码后的 EcmaArray 添加到属性列表中
            properties_.emplace_back(std::move(p));                
            break;  
        }

        // 处理 AMF 对象结束标志
        case kAMFObjectEnd:
        {
            // 返回已解析的字节数
            return parsed;
        }

        // 处理 AMF StrictArray 类型
        case kAMStrictArray:
        {
            // 读取数组的元素数量，并更新已解析的字节数和数据指针
            int count = BytesReader::ReadUint32T(data);
            // 更新已解析字节数，跳过 4 个字节的数量信息
            parsed += 4;

            // 更新数据指针，跳过数量字段
            data += 4;

            // 创建 AMFObject 对象
            std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);

            while (count > 0)
            {
                // 解码数组中的每个元素
                auto len = p->DecodeOnce(data, size - parsed, true);

                // 如果解码失败，返回 -1
                if (len == -1)
                {
                    return -1;
                }

                // 更新数据指针，跳过解码的字节
                data += len;

                // 更新已解析字节数
                parsed += len;

                // 减少剩余元素数量
                count --;
            }

            // 输出解码后的 EcmaArray 的调试信息
            RTMP_TRACE << " EcmaArray : ";

            // 调用每个属性的 Dump() 方法，输出其调试信息
            p->Dump();

            // 将解码后的 EcmaArray 添加到属性列表中
            properties_.emplace_back(std::move(p));    
            break;
        }

        // 处理 AMF Date 类型
        case kAMFDate:
        {
            // 创建 AMFDate 对象并传递属性名称
            std::shared_ptr<AMFDate> p = std::make_shared<AMFDate>(nname);

            // 解码日期属性值
            auto len = p->Decode(data, size - parsed);

            // 如果解码失败，返回 -1
            if (len == -1)
            {
                return -1;
            }

            // 更新数据指针，跳过解码的字节
            data += len;

            // 更新已解析字节数
            parsed += len;

            // 输出解码后的日期值
            RTMP_TRACE << " Date value : " << p->Date();

            // 将解码后的日期属性添加到属性列表中
            properties_.emplace_back(std::move(p));                
            break;   
        }

        // 处理 AMF LongString 类型
        case kAMFLongString:
        {
            // 创建 AMFLongString 对象并传递属性名称
            std::shared_ptr<AMFLongString> p = std::make_shared<AMFLongString>(nname);

            // 解码长字符串属性值
            auto len = p->Decode(data, size - parsed);

            // 如果解码失败，返回 -1
            if (len == -1)
            {
                return -1;
            }

            // 更新数据指针，跳过解码的字节
            data += len;

            // 更新已解析字节数
            parsed += len;

            // 输出解码后的长字符串值
            RTMP_TRACE << " LongString value : " << p->String();

            // 将解码后的长字符串属性添加到属性列表中
            properties_.emplace_back(std::move(p));                
            break;  
        }

        case kAMFMovieClip:

        case kAMFUndefined:

        case kAMFReference:

        case kAMFUnsupported:

        case kAMFRecordset:

        case kAMFXMLDoc:

        case kAMFTypedObject:

        // 处理不支持的 AMF 类型
        case kAMFAvmplus:
        {
            // 输出不支持的 AMF 类型
            RTMP_TRACE << " not surpport type : " << type;
            break;
        }
    }

    return parsed;
}

const AMFAnyPtr &AMFObject::Property(const std::string &name) const
{
    // 遍历 AMF 对象的属性列表
    for (auto const &p : properties_)
    {
        // 如果当前属性的名称与查找的名称匹配
        if (p->Name() == name)
        {
            // 返回找到的属性
            return p;
        }
        // 如果当前属性是一个嵌套的 AMF 对象
        else if (p->IsObject())
        {
            // 将当前属性转换为 AMFObject 对象
            AMFObjectPtr obj = p->Object();

            // 在嵌套的 AMF 对象中查找属性
            const AMFAnyPtr &p2 = obj->Property(name);

            // 如果在嵌套对象中找到属性
            if (p2)
            {
                return p2;
            }
        }
    }

    // 如果没有找到对应的属性，返回一个空的 AMFAnyPtr
    return any_ptr_null;
}

const AMFAnyPtr &AMFObject::Property(int index) const
{
    // 检查索引是否在有效范围内
    if (index < 0 || index >= properties_.size())
    {
        // 如果索引无效，返回一个空的 AMFAnyPtr
        return any_ptr_null;
    }

    // 如果索引有效，返回对应索引处的属性
    return properties_[index];
}

AMFObject::~AMFObject()
{

}