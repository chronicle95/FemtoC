/* Ackerman function calculation */
/* As is, from Prof.Brailsford's presentation on Computerphile video */
/* For this to work past 4,0 the stack section has to be expanded to at least 4 Mb */

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

int ack(int m, int n) {
	int ans = 0;
	if (m == 0) ans = n + 1;
	else if (n == 0) ans = ack (m - 1, 1);
	else ans = ack (m - 1, ack (m, n - 1));
	return ans;
}

int main() {
	for (int i = 0; i < 6; i = i + 1)
	for (int j = 0; j < 6; j = j + 1)
	printf ("ackerman (%i,%i) is: %i%c", i, j, ack(i, j), 10);
}

