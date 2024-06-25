## lss流媒体项目
### 工程目录结构
```c++
lss/
    |-- bin
    |-- build 存放编译产生的中间文件
    |-- src
        |-- base 存放基础函数库，该目录下的源文件将被编译成一个`libbase.a`的库
            |-- CMakeLists.txt 
            |-- TTime.h
            |-- TTime.cpp 时间常用函数，测量事件的时长以及当前时间，通过一个返回当前系统的UTC时间进行计算
            |-- StringUtils.h
            |-- StringUtils.cpp 字符串操作函数，实现字符串前缀和后缀匹配，文件名、文件路径的操作，字符串的分割
        |-- main
        |-- CMakeLists.txt 指定编译的源文件目录，生成程序名，编译完成后执行安装
    |-- CMakeLists.txt  设置全局编译参数，程序输出目录，编译的子目录
    |-- README.md
```