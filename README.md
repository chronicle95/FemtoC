## About
This is a toy compiler for a subset of C programming language.
It does not comply with any of the standards.

The goal of this project is to write a compiler which can be compiled by itself and still resemble all the basic features of the language, so that it can as well be compiled by any mainstream toolset, like gcc or clang.

_Note: parts of this readme may become obsolete at some point and other parts may display some functionality which is not yet implemented but is very desirable at the moment._
## Interface
### Order of execution
The compiler is implemented as a command line utility.

Source code is fed via standard input. The resulting assembly listing is dumped to standard out. Both I/O are in plain ASCII text. No fancy unicode stuff.
### What kind of input
Everything regarding the subset is described in paragraphs below. Refer to the Features section.
### What kind of output
It produces a listing for GNU Assembler (x86_64, v2.34 at the moment of writing this doc) with AT&T syntax.
### Usage
The assembly listing can be directly fed to the assembler and from there the object can be linked to produce ELF file. It may just work on any UNIX-based system, but was only tested on Linux.

FemtoC comes with a few sample programs. I will demonstrate how it works in practice.
Dependencies: make, gcc, binutils.

First, we have to build the compiler itself (the first time).
```
$ make
```
Then, compile one of the provided examples. We pass the source code through a pipe. And then the output is again passed through another pipe to assembler.
```
$ cat examples/hello.fc | ./cc | as
```
The object file by the name ``a.out`` was created. Let's link the object file into final executable and run it.
```
$ ld a.out -o hello
$ ./hello
Hello, World!
```
As you can see, this compiler is very basic in terms of its user experience. Yet again, providing good interface is not the goal of this pet project.
## Language subset features
### Preprocessing
The parser does accept a couple of hashed statements:
- ``include`` does not work, ignored; a dummy put in place so that the program does not freak out when this statement is met;
- ``define`` partially works; it can only accept single constant integers as a means of substitution.

Any other macro statements are not supported. Please use some other tool for preprocessing.
### Keywords
```
int, char, sizeof, if, else, while, for, asm, return, goto, break, continue, NULL
```
_These *can* be used as identifiers, but it would be kinda hard to refer to them later on._
### Data types
Four data types are supported: ``int``, ``int[]``, ``char`` and ``char[]``.

Integers are assumed to be signed and have 64 bits of precision.

Characters are unsigned 8 bit.

Arrays do not preserve information about their size, so performing ``sizeof()`` on an array returns single element size. Arrays are only one-dimensional. To use more than one dimension one must utilize pointers. Arrays are present as a type, but can only be used for static globals.

Both numbers and characters can be pointed to, with a maximum dimensional depth of 3. So, a ``char***`` is still valid, while ``char****`` is not.

Pointers are not considered to be a distinct type internally, even though their type of origin and depth might get checked at the time of assignment. Pointer depth is preserved when an array of pointers is created.
### Variables
You can create global and local variables. Global scope is visible everywhere, while local scope is only seen in the function of declaration.

Every variable must be initialized upon declaration. You can't initialize an array.
Stack storage is limited to 65Kb, so it is advisable to either use static space or preallocate memory in other way.

You can define a variable anywhere inside of a function, but it must be done prior to the usage. Defining a variable inside of a loop is equivalent to defining it at the beginning of a function, except that the variable will be reinitialized every time the statement is encountered.
### Functions
Todo
### Expressions and operators
Todo
### Pointer arithmetic
Todo

