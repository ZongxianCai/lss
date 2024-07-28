#include "TimingWheel.h"
#include "network/base/Network.h"

using namespace lss::network;

// 分别调整了 wheels_ 向量的每个元素的大小，用于任务或事件调度
TimingWheel::TimingWheel()
:wheels_(4)     // 初始化 wheels_ 向量，大小为 4
{
    // 根据不同的时间类型进行调整
    wheels_[kTimingTypeSecond].resize(60);  // 秒
    wheels_[kTimingTypeMinute].resize(60);  // 分钟
    wheels_[kTimingTypeHour].resize(24);    // 小时
    wheels_[kTimingTypeDay].resize(30);     // 天
}

TimingWheel::~TimingWheel()
{
    // TimingWheel 类的成员变量 EntryPtr 是智能指针（std::shared_ptr），会自动管理内存
    // 当 TimingWheel 对象被销毁时，智能指针会自动释放其管理的资源，不需要在析构函数中手动释放资源
}

// 将一个延迟任务插入到以秒为单位的定时轮中的相应位置
void TimingWheel::InsertSecondEntry(uint32_t delay, EntryPtr entryPtr)
{
    // 使用 delay - 1 作为索引来访问 wheels_[kTimingTypeSecond] 向量中的一个元素，随后调用该元素的 insert 方法，将 entryPtr 插入到该元素中
    wheels_[kTimingTypeSecond][delay - 1].insert(entryPtr);
}

// 将一个延迟任务插入到以分钟为单位的定时轮中的相应位置，并在延迟时间到达时调用 InsertEntry 方法
void TimingWheel::InsertMinuteEntry(uint32_t delay, EntryPtr entryPtr)
{
    // 计算剩余的分钟数
    auto minute = delay / kTimingMinute;
    // 计算剩余的秒数
    auto second = delay % kTimingMinute;

    // 使用 std::make_shared 创建了一个名为 newEntryPtr 的 CallbackEntry 对象的共享指针，该 CallbackEntry 对象是通过 lambda 表达式创建的，
    // 该 lambda 表达式捕获了 this 指针、second 和 entryPtr，并在调用时调用 InsertEntry 方法，将 second 和 entryPtr 作为参数传递 
    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr]() {
        InsertEntry(second, entryPtr);
    });

    // 将 newEntryPtr 插入到以分钟为单位的定时轮中的相应位置
    wheels_[kTimingTypeMinute][minute - 1].emplace(newEntryPtr);
}

// 将一个延迟任务插入到以小时为单位的定时轮中的相应位置，并在延迟时间到达时调用 InsertEntry 方法
void TimingWheel::InsertHourEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto hour = delay / kTimingHour;
    auto second = delay % kTimingHour;

    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr]() {
        InsertEntry(second, entryPtr);
    });

    wheels_[kTimingTypeHour][hour - 1].emplace(newEntryPtr);
}

// 将一个延迟任务插入到以天为单位的定时轮中的相应位置，并在延迟时间到达时调用 InsertEntry 方法
void TimingWheel::InsertDayEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto day = delay / kTimingDay;
    auto second = delay % kTimingDay;

    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr]() {
        InsertEntry(second, entryPtr);
    });

    wheels_[kTimingTypeDay][day - 1].emplace(newEntryPtr);
}

// 根据延迟时间将任务插入到相应的时间单位的定时轮中
void TimingWheel::InsertEntry(uint32_t delay, EntryPtr entryPtr)
{
    if (delay <= 0)
    {
        entryPtr.reset();
    }

    if (delay < kTimingMinute)
    {
        InsertSecondEntry(delay, entryPtr);
    }
    else if (delay < kTimingHour)
    {
        InsertMinuteEntry(delay, entryPtr);
    }
    else if (delay < kTimingDay)
    {
        InsertHourEntry(delay, entryPtr);
    }
    else
    {
        auto day = delay / kTimingDay;
        if (day > 30)
        {
            NETWORK_ERROR << " Not support delay more than 30 days ! ";
            
            return;
        }

        InsertDayEntry(delay, entryPtr);
    }
}

// 实现了一个定时轮算法，用于处理不同级别的定时任务
void TimingWheel::OnTimer(int64_t now)
{
    if (last_ts_ == 0)
    {
        last_ts_ = now;
    }

    // ms
    if (now - last_ts_ < 1000)
    {
        return;
    }

    last_ts_ = now;
    ++tick_;

    // 处理秒级定时器
    PopUp(wheels_[kTimingTypeSecond]);

    if (tick_ % kTimingMinute == 0)
    {
        // 处理分钟级定时器
        PopUp(wheels_[kTimingTypeMinute]);
    }
    else if (tick_ % kTimingHour == 0)
    {
        // 处理小时级定时器
        PopUp(wheels_[kTimingTypeHour]);
    }
    else if (tick_ % kTimingDay == 0)
    {
        // 处理天级定时器
        PopUp(wheels_[kTimingTypeDay]); 
    }
}

// 弹出时间轮中的元素
void TimingWheel::PopUp(Wheel &bq)
{
    // bq.front().clear();
    WheelEntry tmp;             // 创建一个临时变量 tmp
    bq.front().swap(tmp);       // 将 bq 中的第一个元素交换到 tmp 中
    bq.pop_front();             // 将 bq 中的第一个元素弹出
    bq.push_back(WheelEntry()); // 将 bq 中的最后一个元素设置为一个空的 WheelEntry 对象

}

// 在指定的延迟时间后执行传入的函数对象cb
void TimingWheel::RunAfter(double delay, const Func &cb)
{
    // 创建了一个名为 cbEntryPtr 的智能指针，指向一个 CallbackEntry 对象
    // CallbackEntry 是一个自定义的类，接收一个函数对象作为构造函数参数
    // 使用 lambda 表达式创建了一个匿名函数，该函数没有参数，但会调用传入的函数对象 cb
    CallbackEntryPtr cbEntryPtr = std::make_shared<CallbackEntry>([cb]() {
        cb();
    });

    // 将匿名函数作为参数传递给 CallbackEntry 的构造函数，创建一个 CallbackEntry 对象，并将其存储在 cbEntryPtr 指向的内存位置
    InsertEntry(delay, cbEntryPtr);
}

// 右值引用：提高性能和资源利用效率，实现完美转发（将传入的函数对象以相同的方式传递给其他函数，可以避免函数重载和模板函数的冗余代码，提高代码的可维护性和灵活性）
// 使用左值引用 (const Func&) ，会导致额外的拷贝构造函数调用，从而产生额外的开销，使用右值引用 (Func&&) 可以避免额外的拷贝操作，直接将函数对象的所有权转移给新创建的 CallbackEntry 对象
void TimingWheel::RunAfter(double delay, Func &&cb)
{
    CallbackEntryPtr cbEntryPtr = std::make_shared<CallbackEntry>([cb]() {
        cb();
    });

    InsertEntry(delay, cbEntryPtr);
}

// 在指定的间隔时间内重复执行传入的函数对象 cb ，并将每次执行的回调函数都存储在 CallbackEntry 对象中
void TimingWheel::RunEvery(double interval, const Func &cb)
{
    CallbackEntryPtr cbEntryPtr = std::make_shared<CallbackEntry>([this, cb, interval]() {
        cb();

        RunEvery(interval, cb);
    });

    // 调用 InsertEntry 函数，将间隔和 cbEntryPtr 作为参数传递进去
    InsertEntry(interval, cbEntryPtr);
}

// 右值引用
void TimingWheel::RunEvery(double interval, Func &&cb)
{
    CallbackEntryPtr cbEntryPtr = std::make_shared<CallbackEntry>([this, cb, interval]() {
        cb();

        RunEvery(interval, cb);
    });

    InsertEntry(interval, cbEntryPtr);
}