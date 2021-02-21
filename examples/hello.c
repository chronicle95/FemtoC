/* Hello World example for FemtoC */

#include <stdio.h>

#if 0
int putchar(char c) {
	asm {
	movq $1, %rax        # syscall = write
	movq $1, %rdi        # file = stdout
	leaq 8(%rbp), %rsi   # buffer = &c
	movq $1, %rdx        # count = 1
	syscall
	}
}

int puts(char *s) {
	while (*s) {
		putchar (*s);
		s = s + 1;
	}
	putchar (10);
}
#endif

int main() {
	puts ("Hello World!");
}
