# klang

### 关于
klang是一个动态类型脚本语言。本项目实现了klang的独立编译器和运行时编译器，高效的，基于寄存器的，可嵌入的字节码解释器。
klang在底层的一些设计上参考了Lua，因此你可能在klang的部分源码中，或与C的交互方式上看到Lua的影子。

### 构建
首先，请确保以下工具可用
1. gcc或者与gcc兼容的编译工具链(比如clang)。并且至少支持ISO C99标准。
2. GNU Make。

然后克隆此仓库，初始化依赖的仓库
```bash
git submodule init
git submodule update
```
如果你没有配置SSH，那么可能会在克隆依赖仓库时失败，此时你可能需要手动克隆。

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
符合POSIX标准的操作系统下应当都可以成功构建(也许需要对Makefile做一些修改)，但我没有尝试过。

### 运行
目前，klang仅是个demo，因此，执行环境有一些限制。

- klangc(klang的独立编译器)没有限制。
- klang(klang的解释器)的工作目录必须在项目根目录下。在Windows下，还需要将``./lib/klang.dll``复制到项目根目录下，以使``klang.exe``能够加载动态库。

这些限制很容易解决，但不是当务之急，于是暂且不管。

### 特性

TODO: 补充语言特色
