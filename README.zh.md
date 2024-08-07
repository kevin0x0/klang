# klang

### 语言

- 英语 [English](README.md)
- 汉语

### 关于
klang是一个动态类型脚本语言。本项目实现了：

- klang的独立编译器``klangc``
- 运行时编译器。与解释器分离，以外部库的形式实现。
- 基于寄存器的，可嵌入的字节码解释器。

### 构建
首先，请确保以下工具可用
1. gcc或者与gcc兼容的编译工具链(比如clang)。并且至少支持ISO C99标准。
2. GNU Make。

然后克隆此仓库，初始化依赖的仓库

```bash
git submodule init
git submodule update
```

如果你没有配置SSH，那么可能会在克隆依赖仓库时失败，此时你可能需要手动克隆依赖。

最后:
- Windows

  使用MinGW构建。在项目根目录执行

```bash
make PLATFORM=Windows
```

- Linux

  在项目根目录执行

```bash
make
```

- 其他平台

  符合POSIX标准的操作系统下应当都可以成功构建(也许需要对``Makefile``做一些修改)，但我没有尝试过。

### 运行环境
在Windows下，需要确保``./lib/klang.dll``能被``klang.exe``加载。
你可以:

- 将``klang.dll``的位置添加到``Path``中
- 将``klang.dll``复制到``klang.exe``所在目录下
- 将``klang.dll``复制到当前工作目录下

建议采用第一种或第二种方式。

### 快速体验

以下假定执行环境为项目的根目录（你可以将可执行文件的位置加入搜索路径中以在任何位置运行）。

构建完成后，在项目根目录下执行

```bash
./bin/klang
```

即可进入交互式模式。

交互式模式下，可输入任意语句。该模式下语句必须以分号结尾(TODO: 添加简单的语法介绍)。如果是表达式语句，会打印表达式的值。

```bash
>>> [ i * i | i = 0, 10 ];
[0, 1, 4, 9, 16, 25, 36, 49, 64, 81]
>>> 
```

```bash
>>> [ i | i = 0, 100; isprime i ] where local isprime(n) => {
...   if n <= 2: return n == 2;
...   for i = 2, n:
...     if n % i == 0: return false;
...   return true;
... };
[2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97]
>>> 
```


在根目录下执行

```bash
./bin/klang <script>
```

可以从文件中加载脚本，编译并执行。
在项目根目录的``./samples``目录下，有一些预先写好的示例程序可供体验。

如果从文件中加载的脚本在执行时出错，backtrace能够打印详细的调用栈信息。

如果你对klang的字节码感兴趣，可以尝试klang的独立编译器klangc。在项目根目录下执行

```bash
./bin/klangc <script> -p <output>
```

这会从``<script>``中加载并编译脚本，然后将字节码以人类可读的形式打印到文件``<output>``中。

``<output>``和``<script>``都可缺省， 如果``<output>``缺省，则打印到标准输出。如果``<script>``缺省，则会从标准输入读取脚本。

### 特性

TODO: 补充语言特色
