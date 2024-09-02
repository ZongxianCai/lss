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
        |-- network/ 高性能网络库的实现
            |-- base/ 基础功能
                |-- Network.h 定义日志信息
                |-- InetAddress.h
                |-- InetAddress.cpp 网络地址相关操作
                |-- SocketOpt.h
                |-- SocketOpt.cpp Socket相关操作
                |-- MsgBuffer.h
                |-- MsgBuffer.cpp 消息缓冲区相关操作
            |-- net/ 网络协议
                |-- tests/ 测试目录
                    |-- CMakeLists.txt 编译可执行文件并设置路径
                    |-- EventLoopThreadTest.cpp 测试事件循环线程的功能
                    |-- InetAddressTest.cpp 测试网络地址相关的操作
                    |-- SocketOptTest.cpp 测试Socket通信
                    |-- AcceptorTest.cpp 测试Acceptor
                    |-- TcpConnectionTest.cpp 测试TcpConnection
                    |-- TcpServerTest.cpp 测试TcpServer
                    |-- DnsServiceTest.cpp 测试DnsService
                    |-- TcpClientTest.cpp 测试TcpClient
                    |-- UdpClientTest.cpp 测试UdpClient
                    |-- UdpServerTest.cpp 测试UdpServer
                |-- EventLoop.h 
                |-- EventLoop.cpp 实现事件循环的逻辑，用于处理网络事件
                |-- Event.h
                |-- Event.cpp 处理与事件循环相关的事件
                |-- EventLoopThread.h
                |-- EventLoopThread.cpp 创建一个在单独线程中运行的事件循环
                |-- PipeEvent.h
                |-- PipeEvent.cpp 处理与管道有关的事件
                |-- EventLoopThreadPool.h
                |-- EventLoopThreadPool.cpp 事件循环线程池处理事件
                |-- TimingWheel.h
                |-- TimingWheel.cpp 以时间轮的方式设置定时任务
                |-- Acceptor.h
                |-- Acceptor.cpp 接收连接相关的操作
                |-- Connection.h
                |-- Connection.cpp 网络连接相关的操作
                |-- TcpConnection.h
                |-- TcpConnection.cpp TCP相关操作
                |-- UdpSocket.h
                |-- UdpSocket.h UDP Socket相关操作
            |-- CMakeLists.txt 指定编译的文件目录
            |-- TcpServer.h
            |-- TcpServer.cpp TCP Server相关操作
            |-- DnsService.h
            |-- DnsService.cpp DNS Service相关的操作
            |-- TcpClient.h
            |-- TcpClient.cpp TCP Client相关操作
            |-- UdpClient.h
            |-- UdpClient.cpp UDP Client相关操作
            |-- UdpServer.h
            |-- UdpServer.cpp UDP Server相关操作
            |-- TestContext.h
            |-- TestContext.cpp Context上下文相关操作
        |-- mmedia/
            |-- Packet.h
            |-- Packet.cpp 多媒体数据包相关操作
            |-- MMediaHandler.h 抽象基类，只定义纯虚函数
        |-- main/
            |-- CMakeLists.txt 指定编译所需的依赖文件
            |-- main.cpp 模块测试
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

**注意：链接jsoncpp时，需要在/src/base/CMakeLists.txt中添加target_link_libraries(base_test base jsoncpp_static.a)**

---------------------------------------------------------------------------------------------------------------------------------

### 高性能网络框架设计与实现

高性能服务器：

> - 性能表现：`吞吐量大`、`延时低`、`资源使用率低`；
> - 影响性能的因素：`上下文切换（线程切换、系统调用、锁）`、`内存拷贝`、`多线程/多进程`。

流媒体服务器：

> - 特点：
>   - 长连接；
>   - 并发量大：
>       - 万兆网卡的最大传输速率：10 000 000 000 bps；
>       - 一般视频流的码率：2 500 000 bps；
>       - 跑满网卡的80%（预留20%给其他程序通信）：并发数 = 10 000 000 000 * 0.8 / 2 500 000 = 3200。
>   - 数据量大：
>       - 跑满网卡的80%（预留20%给其他程序通信）：数据量 = 10 000 000 000 * 0.8 = 8 000 000 000 bps
>   - 平均延时低。
> - 编程框架：
>   - 网络库接收数据，经过流媒体模块进行解析，最后由直播业务模块处理；
>   - 直播业务进行请求发送，流媒体模块封装，最后由网络库进行发送；
>   - 网络库是整个系统的输入输出，网络库的性能决定整个系统的性能。

---------------------------------------------------------------------------------------------------------------------------------

高性能网络库：

> - 功能：
>   - 事件循环：IO就绪事件监听，IO事件处理，事件管理；
>   - 任务执行：任务入队，任务执行；
>   - 定时任务：定时任务检测，定时任务执行。
> - 选型：
>   - 同步IO模型    -->     IO复用模型；
>   - Reactor模式   -->     事件循环在一个工作线程内；
>   - 线程池        -->     每个线程是一个事件循环。
> - 针对性能的优化：
>   - 事件在同一个线程内循环    -->     减少线程的切换以及锁的使用；
>   - 减少IO调用               -->     分散读，聚集写；
>   - 减少内存拷贝             -->      发送队列只保存地址和长度。
> - 事件循环：
>   - IO就绪事件监听；
>   - IO事件处理；
>   - 事件管理。
> - **EventLoop**的特性：
>   - 一个线程只有一个EventLoop     -->     通过线程局部变量实现；
>   - EventLoop中的Loop一定是在其所在的线程中运行；
>   - EventLoop中的Loop通过epoll实现IO就绪事件监听。
> - **Event**：
>   - Epoll处理的是文件描述符的就绪事件；
>   - IO事件处理函数和文件描述符抽象成一个事件类Event；
>   - 通过epoll_event的data私有数据成员将Event和epoll_event绑定在一起。
> - **Event**的设计：
>   - Event是一个基类，主要定义事件接口；
>   - Event可以处理的事件：读、写、关闭、出错；
>   - Event事件只在同一个EventLoop循环；
>   - Event事件由EventLoop管理。
> - **EventLoopThread**的设计：
>   - EventLoopThread通过创建一个std::thread线程来运行EventLoop；
>   - EventLoopThread只运行一个EventLoop；
>   - EventLoopThread保证EventLoop的生命周期与std::thread相同。
> - 线程池：
>   - 利用多个CPU或者多个核，提高并发能力，提高服务器性能；
>   - 统一管理线程，有利于利用CPU性能；
>   - 线程池预先创建好线程，减少启动时间；
>   - 线程池最终对外提供EventLoop来使用线程池的并发能力。
> - **EventLoopThreadPool**的设计：
>   - 绑定CPU：std::thread运行的CPU不确定；
>   - 使用Linux提供的接口pthread_setaffinity_np将线程绑定到指定的CPU上；
>   - 配置起始CPU和线程数量。
> - 任务队列：
>   - EventLoop提供执行任务的功能；
>   - EventLoop执行一个任务，有两种情况：
>       - 1.调用方所在线程和EventLoop所在线程是同一个线程，则直接执行；
>       - 2.调用方所在线程和EventLoop所在线程不是同一个线程，任务入队，由Loop执行。
>   - 任务队列需要加锁。
> - **TimingWheel**定时任务：
>   - 每个EventLoop都有一个TimingWheel，定时任务只在自己的EventLoop内循环；
>   - TimingWheel固定一秒转一次；
>   - 定时任务提供天、小时、分钟、秒4种单位的时间刻度。
> - 网络地址类：
>   - 网络编程经常需要用到IP和端口，传递两个值比较麻烦；
>   - IP和端口经常需要转换成其他形式；
>   - 有时候需要对地址进行分类检测；
>   - InetAddress类方便存储IP和端口信息，提供地址相关的操作。
> - **Accept**：
>   - TCP服务器由Accept接收新连接；
>   - Accept每次从接收队列中取出第一个请求，接收队列中都是完成了三次握手的请求；
>   - 设置SO_REUSEPORT后，多线程可以同时监听同一个地址和端口。
> - **非阻塞Accept**：
>   - 非阻塞的监听sockfd，会马上返回，没有连接就会返回EAGAIN；
>   - 非阻塞的监听套接字配合epoll使用；
>   - 有新连接，epoll返回读就绪事件；
>   - 边缘触发模式下，一次读事件要一直读到返回EAGAIN错误为止；
>   - Acceptor是一个Event的子类，主要处理读事件。
> - **网络连接**：
>   - 一个TCP连接由四元组决定：（local ip、local port、remote ip、remote port）；
>   - UDP也可以是连接的，但是这个连接只是由内核记录IP和端口；
>   - 一个连接有很多操作：可读、可写、可关闭。
> - **Connection**的设计：
>   - Connection保存连接的相关信息：套接字，local address、remote address；
>   - Connection可以增加一些连接相关的私有数据；
>   - Connection存在空闲状态，需要主动去激活Connection；
>   - Connection是Event的一个子类。
> - **TcpConnection**的设计：
>   - TcpConnection代表一个TCP连接，由TCP服务器创建；
>   - TcpConnection是Connection的子类，也是Event的子类；
>   - TcpConnection处理套接字的所有IO事件；
>   - TcpConnection的IO处理函数都需要在EventLoop循环中；
>   - 处理套接字IO事件：
>       - 读事件：有数据来，读数据后通过回调通知上层处理数据；
>           - TcpConnection是面向字节流的连接，字节流本身没有边界；
>           - TcpConnection的读事件由EventLoop监听，数据由TcpConnection读，并调用回调处理；
>           - 套接字读事件使用了epoll边缘触发模式，一次触发需要一直读到没有数据为止；
>           - 使用readv接口，可以减少读IO的调用。
>       - 写事件：保存要发送的数据，可以发送数据，从队列中取出数据进行发送，直到发送完队列中的数据，通过回调通知上层；
>           - 业务层会调用TcpConnection的发送接口发送数据，实际的发送由TcpConnection完成；
>           - TcpConnection把队列中的数据发送完，调用回调通知上层；
>           - TcpConnection只保存要发送的数据地址和长度，不拷贝数据，并且认为数据的生命周期足够长，直到数据发送完；
>           - TcpConnection的发送函数可能由其他线程调用，要保证发送事件在TcpConnection所在的EventLoop中；
>           - 通过writev可以减少写IO的调用。
>       - 关闭事件：关闭连接，通知上层关闭连接；
>           - EPOLLHUP表示描述符的一端或两端已经关闭或挂断，或者被其他错误关闭；
>           - 关闭事件除了关闭描述符外，还需要通知上层业务，告知连接关闭；
>           - 有可能描述符关闭，但是仍执行任务队列和超时检测，所以需要一个状态位来表示连接已经关闭；
>           - TcpConnection提供强制关闭连接的接口，由业务层调用。
>       - 出错事件：打印出出错信息，关闭连接，通知上层关闭连接；
>           - EPOLLERR表示描述符遇到错误，套接字变成可读；
>           - 通过getsockopt函数的SO_ERROR选项可以读取错误号；
>           - 出错的套接字需要回收资源。
>       - 超时事件：关闭连接，通知上层连接超时。
>           - 闲置的TcpConnection是一种浪费，需要关闭，释放资源；
>           - TcpConnection的超时通过定时任务来触发；
>           - 超时后，直接把TcpConnection关闭，并通知上层业务；
>           - 只要有数据交互，就延长TcpConnection的生命周期；
>           - TcpConnection通过增加定时任务智能指针的引用来延长生命周期。
> - **MsgBuffer**：
>   - 源文件来源于陈硕的muduo；
>   - TCP的数据是字节流，读取到的数据，不足一个消息，需要持有不足一个消息的数据；
>   - MsgBuffer实现了一个环形缓冲区，先进先出；
>   - MsgBuffer提供了安全写和便捷读的功能。
> - **TcpServer**：
>   - TcpServer通过Acceptor接收客户端连接，创建TcpConnection，并通知业务层；
>   - TcpServer管理所有的TcpConnection，包括分配资源，回收资源，加入事件循环，退出事件循环，设置各种回调等；
>   - TcpServer提供操作TcpConnection的接口，是业务层与连接的中间层；
> - **DnsService**：
>   - 本地DNS缓存的原因，域名在一定的时间内解析不会更新；
>   - 同一个域名不必多次请求解析；
>   - 后台统一解析能减少响应时间；
>   - 域名的更新时间和频率可以通过配置文件设置。
> - **TcpClient**：
>   - TcpClient是TcpConnection的一个子类；
>   - TcpClient主动发起连接，TcpClient和Server之间只存在一个连接；
>   - TcpClient需要处理非阻塞连接，并且管理连接状态；
>   - TcpClient连接成功后，通过TcpConnection处理IO事件；
>   - TcpClient负责注册事件，删除事件；
>   - 非阻塞连接：
>       - 非阻塞的connect会马上返回，返回值有3种情况：
>           - 返回0，表示连接成功；
>           - 返回-1，并且错误号为EINPROGRESS，表示连接中；
>           - 返回-1，错误号不为EINPROGRESS，表示出错。
>       - 连接中的套接字变成可读可写，如果不是出错，则表示连接成功。
> - **UdpSocket**：
>   - UDP面向数据报，报文是有边界的；
>   - UdpSocket是Connection的一个子类；
>   - UdpSocket负责处理UDP套接字的IO事件。
> - **UdpClient**：
>   - UdpClient是UdpSocket的一个子类；
>   - UdpClient可以通过调用Connect来记录服务端IP和端口；
>   - UdpClient负责注册事件，删除事件。
> - **UdpServer**：
>   - UdpServer是UdpSocket的一个子类；
>   - UdpServer接收不同客户端的数据，通过客户端的IP和端口区分数据包；
>   - UdpServer负责注册事件，删除事件。

---------------------------------------------------------------------------------------------------------------------------------

### 多媒体模块

> - **有限状态机：**
>   - 在协议解析上的应用：
>       - 表示有限个状态以及在这些状态之间的转移和动作等行为的数学模型；
>       - 状态有限，但是最少两个状态；
>       - 状态转移，既定的条件满足，状态才会发生转移；
>       - 动作是条件满足后状态转移执行的动作；
>       - 其作用主要是描述对象在它的生命周期内所经历的状态序列，以及如何响应来自外界的各种事件。
>   - 实现：
>       - 了解起始、终止、现态、目标状态、动作、条件；
>       - 识别状态，定义状态；
>       - 定义状态转移，明确转移条件；
>       - 定义每个状态转移执行的动作；
>       - 实现控制器。
> - **上下文Contex：**
>   - 程序执行依赖的变量的集合；
>   - 承上启下；
>   - 同时用到多个变量；
>   - 中断后需要再次执行；
>   - 实例TestContext：
>       - 测试协议：长度+字符串；
>       - 协议的实现需要用到有限状态机；
>       - 消息长度需要保存变量；
>       - 解析出来的消息，需要调用回调通知上层。
> - **多媒体数据包Packet：**
>   - 流媒体协议的数据包内容主要包含：音频包、视频包、Meta包、其他数据包；
>   - 存储流媒体数据；
>   - 分配内存和释放内存；
>   - 判断包的类型。
> - **多媒体模块协议回调类（MMediaHandler）：**
>   - 抽象基类，只定义纯虚函数；
>   - 由于多媒体模块实现的流媒体协议，只负责解析和封装协议，并且多媒体数据包需要集中管理，所以，多媒体数据包由直播业务模块进行管理；
>   - 多媒体模块的协议数据回调：
>       - 把回调函数都集中在一起，抽象一个接口类出来;
>       - 优势：一方面，回调都集中在一起，传递一个基类指针就能调用所有的回调；另一方面，每种协议需要的回调可能不一样，只需要继承这个基类，加上自己需要的回调就可以。