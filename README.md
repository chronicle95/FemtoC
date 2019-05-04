## FemtoC

An attempt at making a C compiler just for fun.

It is restricted in many ways and is still work in progress.

### Build

`$ make`

### Run

It expects the program on `stdin` and produces assembly code to `stdout`.

So to compile a program you can do pipelining:
`$ cat hw.fc | ./cc > hw.s`

And to execute the assembly, use the supplied runtime:
`$ ./rr hw.s`
