# klang

### Language
> Note: This README was translated from [Chinese](README.zh.md)  by AI.

- English
- Chinese [汉语](README.zh.md)

### About
klang is a dynamically-typed scripting language. This project implements:

- an independent compiler for klang, `klangc`
- a runtime compiler, implemented as an external library, separate from the interpreter
- a register-based, embeddable bytecode interpreter

### Building
First, make sure the following tools are available:
1. gcc or a gcc-compatible compiler toolchain (such as clang), supporting at least the ISO C99 standard.
2. GNU Make.

Then, clone this repository and initialize the dependent repositories:

```bash
git submodule init
git submodule update
```

If you haven't configured SSH, the clone of the dependent repositories may fail, and you may need to clone them manually.

Finally:
- Windows:
  Use MinGW to build. In the project root directory, execute:

  ```bash
  make PLATFORM=Windows
  ```

- Linux:
  In the project root directory, execute:

  ```bash
  make
  ```

- Other platforms:
  The build should work on any POSIX-compliant operating system (though some Makefile modifications might be necessary), but I haven't tried it.

### Runtime Environment
On Windows, make sure that `./lib/klang.dll` can be loaded by `klang.exe`.
You can:

- Add the location of `klang.dll` to the `Path`
- Copy `klang.dll` to the same directory as `klang.exe`
- Copy `klang.dll` to the current working directory

The first or second option is recommended.

### Quick Experience

Assuming the execution environment is the project root directory (you can add the executable's location to the search path to run it from anywhere).

After the build is complete, in the project root directory, execute:

```bash
./bin/klang
```

to enter the interactive mode.

In the interactive mode, you can enter any statement. Statements in this mode must end with a semicolon (TODO: add a simple syntax introduction). If the statement is an expression, its value will be printed.

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

To execute a script from a file, in the root directory, run:

```bash
./bin/klang <script>
```

This will load the script from the file, compile it, and execute it.
In the `./samples` directory of the project root, there are some pre-written example programs for you to try.

If there is an error when executing a script loaded from a file, the backtrace will print detailed call stack information.

If you're interested in klang's bytecode, you can try the independent klang compiler, klangc. In the project root directory, execute:

```bash
./bin/klangc <script> -p <output>
```

This will load and compile the script from `<script>`, and then print the bytecode in a human-readable form to the file `<output>`.

`<output>` and `<script>` can both be omitted. If `<output>` is omitted, the bytecode will be printed to standard output. If `<script>` is omitted, the script will be read from standard input.

### Features

TODO: Add language features
