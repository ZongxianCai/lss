## lss流媒体项目
### 工程目录结构
```c++
lss/
    |-- bin/
    |-- build/ 存放编译产生的中间文件
    |-- src/
        |-- base/ 存放基础函数库，该目录下的源文件将被编译成一个`libbase.a`的库
            |-- CMakeLists.txt 
            |-- TTime.h
            |-- TTime.cpp 时间常用函数，测量事件的时长以及当前时间，通过一个返回当前系统的UTC时间进行计算
            |-- StringUtils.h
            |-- StringUtils.cpp 字符串操作函数，实现字符串前缀和后缀匹配，文件名、文件路径的操作，字符串的分割
            |-- NonCopyable.h 不可移动和复制的`基类`
            |-- Singleton.h 单例模式
            |-- TestSingleton.cpp 测试单例模式
            |-- Task.h 定时任务头文件
            |-- Task.cpp 定时任务方法的实现
        |-- main/
        |-- CMakeLists.txt 指定编译的源文件目录，生成程序名，编译完成后执行安装
    |-- CMakeLists.txt  设置全局编译参数，程序输出目录，编译的子目录
    |-- README.md
```
### 通用基础函数库的实现
<br>
实现单例模式，保证一个类仅有一个实例（只能自行创建实例；多线程初始化竞争，保证只创建一个实例；不可复制和移动），并提供一个访问实例的全局访问点。
<br>
C++的`三/五法则`：拷贝构造函数、拷贝赋值运算符、析构函数、移动构造函数、移动赋值运算符。实现了上述5个法则中的任意一个，编译器会默认合成另外的4个法则，反之删除其中任意一个法则，编译器则不会默认合成另外的4个法则。
<br>
---------------------------------------------------------------------------------------------
<br>
定时任务：在规定的时间执行；可以单次执行，也可以重复执行；通过回调函数执行任务。
<br>
定时任务的运行（由谁检测任务时间？由谁执行定时任务？）--> 定时任务管理器：存放所有的定时任务；添加定时任务；删除定时任务；检测每个任务是否到点执行；执行任务；定时任务为全局唯一
<br>
-----------------------------------------------------------------------------------------------