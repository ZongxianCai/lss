#pragma once
#include <memory>
#include <functional>
#include <cstdint>
#include <vector>
#include <deque>
#include <unordered_set>

namespace lss
{
    namespace network
    {
        // EntryPtr 是 std::shared_ptr<void> 的别名，用于管理动态分配的内存资源，void 表示这个智能指针可以指向任意类型的对象
        using EntryPtr = std::shared_ptr<void>;

        // WheelEntry 是 std::unordered_set<EntryPtr> 的别名，用于存储不重复的元素，WheelEntry 是一个无序集合，其中的元素类型是 EntryPtr
        using WheelEntry = std::unordered_set<EntryPtr>;

        // Wheel 是 std::deque<WheelEntry> 的别名，用于在两端进行高效的插入和删除操作，Wheel 是一个双端队列，其中的元素类型是 WheelEntry
        using Wheel = std::deque<WheelEntry>;

        // Wheels 是 std::vector<Wheel> 的别名，用于存储可变大小的元素序列，Wheels 是一个动态数组，其中的元素类型是 Wheel
        using Wheels = std::vector<Wheel>;

        // Func 是 std::function<void()> 的别名，用于存储和调用各种可调用对象（函数、函数指针、成员函数指针等），Func是一个函数类型，接收无参数并且没有返回值
        using Func = std::function<void()>;

        // 一分钟的秒数，用于实现定时轮算法的相关功能
        const int kTimingMinute = 60;

        // 一小时的秒数，用于实现定时轮算法的相关功能
        const int kTimingHour = 60 * 60;

        // 一天的秒数，用于实现定时轮算法的相关功能
        const int kTimingDay = 60 * 60 * 24;

        enum TimingType
        {
            kTimingTypeSecond = 0,  // 秒级定时类型
            kTimingTypeMinute = 1,  // 分钟级定时类型
            kTimingTypeHour = 2,    // 小时级定时类型
            kTimingTypeDay = 3      // 天级定时类型
        };

        class CallbackEntry
        {
        public:
            // 构造函数，接收一个 Func 类型的参数 cb，将其赋值给成员变量 cb_
            CallbackEntry(const Func &cb) : cb_(cb) {}

            // 析构函数，当 CallbackEntry 对象被销毁时调用
            ~CallbackEntry()
            {
                // 检查 cb_ 是否为空，如果不为空，则调用回调函数 cb_()
                if (cb_)
                {
                    cb_();
                }
            }

        private:
            // 存储回调函数
            Func cb_;
        };

        // CallbackEntryPtr 是一个指向 CallbackEntry 类对象的 std::shared_ptr 智能指针的别名，用于创建和管理 CallbackEntry 对象的共享指针
        // 使用共享指针可以自动管理对象的生命周期，避免内存泄漏和悬空指针的问题
        using CallbackEntryPtr = std::shared_ptr<CallbackEntry>;

        class TimingWheel
        {
        public:
            // 构造函数，创建一个空的 TimingWheel 对象
            TimingWheel();

            // 析构函数，释放 TimingWheel 对象占用的资源
            ~TimingWheel();

            // 插入一个延迟为 delay 的条目 entryPtr
            void InsertEntry(uint32_t delay, EntryPtr entryPtr);

            // 定时器触发时调用的函数，传入当前时间 now
            void OnTimer(int64_t now);

            // 将 bq 中的条目弹出
            void PopUp(Wheel &bq);

            // 延迟 delay 后执行回调函数 cb
            void RunAfter(double delay, const Func &cb);

            // 延迟 delay 后执行右值引用的回调函数 cb
            void RunAfter(double delay, Func &&cb);

            // 每隔 interval 执行一次回调函数 cb
            void RunEvery(double interval, const Func &cb);

            // 每隔 interval 执行一次右值引用的回调函数 cb
            void RunEvery(double interval, Func &&cb);

        private:
            // 插入一个延迟为 delay 的秒级条目 entryPtr
            void InsertSecondEntry(uint32_t delay, EntryPtr entryPtr);

            // 插入一个延迟为 delay 的分钟级条目 entryPtr
            void InsertMinuteEntry(uint32_t delay, EntryPtr entryPtr);

            // 插入一个延迟为 delay 的小时级条目 entryPtr
            void InsertHourEntry(uint32_t delay, EntryPtr entryPtr);

            // 插入一个延迟为 delay 的天级条目 entryPtr
            void InsertDayEntry(uint32_t delay, EntryPtr entryPtr);

            // 时间轮
            Wheels wheels_;

            // 上次触发定时器的时间戳，默认为 0
            int64_t last_ts_{0};

            // 定时器的滴答数，默认为 0
            uint64_t tick_{0};
        };
    }
}