## FemtoC

An attempt at making a C compiler just for fun.

It is restricted in many ways and is still work in progress.

### Build

`$ make`

### Run

Pipeline the source into the compiler:

`$ cat <filename> | ./cc`

It expects the program on `stdin` and produces assembly code to `stdout`.
