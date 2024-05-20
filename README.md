## 动态类型脚本语言klang及其基于寄存器的解释器实现

目前只是个demo，因此执行环境有一些限制，程序的工作目录必须是该项目根目录。
在Windows下，还要将./lib下的klang.dll复制到项目根目录下。

### 构建

克隆此仓库，在项目根目录下执行
```bash
    git submodule init
    git submodule update
```
然后:
- Windows
使用MinGW。在项目根目录执行
```bash
    make PLATFORM=Windows
```

- Linux
在项目根目录执行
```bash
    make
```
