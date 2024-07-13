## lss流媒体项目
### 工程目录结构
```c++
lss/
    |-- bin/
        |-- config/ 存放配置文件
            |-- config.json 配置文件
        |-- log/ 手动创建存放log的目录
    |-- build/ 存放编译产生的中间文件
    |-- third_party/ 存放第三方库
        |-- jsoncpp/
    |-- src/
        |-- base/ 存放基础函数库，该目录下的源文件将被编译成一个`libbase.a`的库
            |-- CMakeLists.txt 
            |-- TTime.h
            |-- TTime.cpp 时间常用函数，测量事件的时长以及当前时间，通过一个返回当前系统的UTC时间进行计算
            |-- StringUtils.h
            |-- StringUtils.cpp 字符串操作函数，实现字符串前缀和后缀匹配，文件名、文件路径的操作，字符串的分割
            |-- NonCopyable.h 不可移动和复制的`基类`
            |-- Singleton.h 
            |-- TestSingleton.cpp 测试单例模式
            |-- Task.h 
            |-- Task.cpp 定时任务方法的实现
            |-- TaskManager.h 
            |-- TaskManager.cpp 定时任务管理器方法的实现
            |-- TestTask.cpp 测试定时任务方法
            |-- Logger.h 
            |-- Logger.cpp 日志输出实现
            |-- LogStream.h 
            |-- LogStream.cpp 构建和发送日志消息实现
            |-- TestLog.cpp 测试日志
            |-- FileLog.h
            |-- FileLog.cpp 日志文件，将日志写入文件并进行切分
            |-- FileManager.h
            |-- FileManager.cpp 日志文件管理
            |-- Config.h
            |-- Config.cpp Log的配置文件
        |-- main/
        |-- CMakeLists.txt 指定编译的源文件目录，生成程序名，编译完成后执行安装
    |-- CMakeLists.txt  设置全局编译参数，程序输出目录，编译的子目录
    |-- README.md
```
### 通用基础函数库的实现

---------------------------------------------------------------------------------------------------------------------------------

实现单例模式，保证一个类仅有一个实例（只能自行创建实例；多线程初始化竞争，保证只创建一个实例；不可复制和移动），并提供一个访问实例的全局访问点。

C++的`三/五法则`：拷贝构造函数、拷贝赋值运算符、析构函数、移动构造函数、移动赋值运算符。实现了上述5个法则中的任意一个，编译器会默认合成另外的4个法则，反之删除其中任意一个法则，编译器则不会默认合成另外的4个法则。

---------------------------------------------------------------------------------------------------------------------------------

定时任务：在规定的时间执行；可以单次执行，也可以重复执行；通过回调函数执行任务。

定时任务的运行（由谁检测任务时间？由谁执行定时任务？）--> 定时任务管理器：存放所有的定时任务；添加定时任务；删除定时任务；检测每个任务是否到点执行；执行任务；定时任务为全局唯一。

定时任务管理器的算法：

> - **直接遍历（容器：unordered_set；插入：O(1)；遍历：O(n)）；**
>   - 实现简单；
>   - 任务事件重新更新后不需要调整任务列表；
>   - 全局定时任务数量少，局限于后续网络库中与事件循环绑定的局部定时任务；
> - 最小时间堆：遍历任务，总是取堆顶任务进行判断，如果堆顶任务未到时间，退出遍历，否则继续执行任务，并设置下一个时间点，随后重建堆，保证堆顶时间为最小的时间（容器：vector、std::make_heap、std::push_heap、std::pop_heap；插入：O(logn)；遍历：不稳定任务未到时间，直接退出，O(1)，如果所有任务到了时间，需要继续执行任务，并设置下一个时间点，随后重建堆，O(nlogn)）；
> - 时间轮：以固定的时间间隔执行当前指向的任务，最右边的时间轮转动一圈后，左边的时间轮转动一格，以此类推（容器：vector、queue、unordered_set；插入：O(1)；遍历：O(1)）。

---------------------------------------------------------------------------------------------------------------------------------

日志的用途：

> - 查问题：程序逻辑、业务逻辑；
> - 输出业务信息： 负载、带宽、 access log。


日志库的设计：

> - **Logger：日志输出**
>   - 日志级别：trace、debug、info、warning、error；
>   - 日志文件：负责具体的日志输出。
> - **LogStream: 日志格式**
>   - 时间格式：YYYY-MM-ddTHH:mm:ss；
>   - 打印级别：TRACE、DEBUG、INFO、WARNING、ERROR。
>   - 文件位置：__FILE__, __LINE__；
>   - 函数名：__func__；
>   - 完整格式：2024-7-10T15:49:00 7821 DEBUG [RtmpContext.cpp:621][Parse] xxxx。
> - **FileLog: 日志文件**
>   - 单个日志不上锁，使用append以原子锁的方式进行写入；
>   - 负责把日志信息写道文件；
>   - 负责完成文件切分。
> - **FileManager: 日志文件管理**
>   - 日志文件申请与释放；
>   - 日志文件的切分检测。

---------------------------------------------------------------------------------------------------------------------------------

配置文件：

> - **说明：**
>   - 系统用到的一些参数：线程数量、打印级别、服务器IP和端口号；
>   - 业务用到的一些参数：HLS支持、RTMP支持、FLV支持、帧队列长度、录制、时移等。
>   - 多个配置文件
> - **管理：**
>   - 启动加载；
>   - 热更新；
>   - 配置文件管理：本地配置、主动更新、被动更新；
>   - 文件格式：(INI、XML、`JSON`、YAML、TOML等)；
> - **JSON库配置：**
>   - 1. 新建`/bin/config/`目录并创建`config.json`；
>   - 2. 在`lss/`根目录下新建`third_party/`并下载`jsoncpp/`；
>   - 3. 修改根目录下的`CMakeLists.txt`，添加`jsoncpp`的编译；
>   - 4. 将`jsoncpp`中`test`模块的编译改为`OFF`；
>   - 5. 链接`jsoncpp`静态库`jsoncpp_static.a`。
> - **Config: log的配置文件**
>   - 加载和解析配置文件；
>   - 从JSON对象中提取日志级别、文件名和路径信息；
>   - 允许外部访问解析后的日志信息。
> - **ConfigManager: 热更新（在Config中实现）**
>   - 通过接口返回config的智能指针；
>   - 加载新配置时，创建新的config智能指针替换原有的智能指针，使其相互独立，相互不产生影响；
>   - 智能指针的赋值线程不安全，需要加锁。

**注意：在bin目录下手动创建log目录**
**注意：编译jsoncpp时，需要在/src/base/CMakeLists.txt中添加target_link_libraries(base_test base jsoncpp_static.a)**

---------------------------------------------------------------------------------------------------------------------------------