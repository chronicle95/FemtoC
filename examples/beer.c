/* 99 bottles song for FemtoC */

#include <stdio.h>

#if 0
int putc(char c) {
	asm {
	movq $1, %rax        # syscall = write
	movq $1, %rdi        # file = stdout
	leaq 8(%rbp), %rsi   # buffer = &c
	movq $1, %rdx        # count = 1
	syscall
	}
}

int putn(int n) {
	char buf[10];
	char *ptr=buf;
	if (n == 0) {
		putc ('0');
		return 0;
	}
	while (n > 0) {
		*ptr = (char) n % 10 + '0';
		ptr = ptr + 1;
		n = n / 10;
	}
	while (ptr != buf) {
		ptr = ptr - 1;
		putc (*ptr);
	}
}

int puts(char *s) {
	while (*s) {
		putc (*s);
		s = s + 1;
	}
}

int printf(char *fmt) {
	int *arg = (int*) &fmt + 1;
	while (*fmt) {
		if (*fmt == '%') {
			fmt = fmt + 1;
			if (*fmt == 'c') {
				putc (*arg);
			} else if (*fmt == 's') {
				puts (*arg);
			} else if (*fmt == 'i') {
				putn (*arg);
			}
			arg = arg + 1;
		} else {
			putc (*fmt);
		}
		fmt = fmt + 1;
	}
}
#endif

int main() {
	int k = 99;
	while (k > 0) {
		printf ("%i bottles of beer on the wall,%c", k, 10);
		printf ("%i bottles of beer.%c", k, 10);
		printf ("Take one down, pass it around,%c", 10);
		k = k - 1;
		if (k != 0) {
			printf ("%i bottles of beer on the wall.%c%c", k, 10, 10);
		} else {
			printf ("No more bottles of beer on the wall.%c%c", 10, 10);
		}
	}
}

