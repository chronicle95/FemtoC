/* Bubblesort example for FemtoC */

#include <stdio.h>

#if 0
char getchar() {
	char c = 0;
	asm {
	movq $0, %rax        # syscall = read
	movq $0, %rdi        # file = stdin
	leaq -24(%rbp), %rsi # buffer = &c
	movq $1, %rdx        # size = 1
	syscall
	}
	return c;
}

int putchar(char c) {
	asm {
	movq $1, %rax        # syscall = write
	movq $1, %rdi        # file = stdout
	leaq 8(%rbp), %rsi   # buffer = &c
	movq $1, %rdx        # size = 1
	syscall
	}
}

int puts(char *s) {
	while (*s) {
		putchar (*s);
		s = s + 1;
	}
}

int gets(char *s) {
	char c = 0;
	while (c != 10) {
		c = getchar ();
		*s = c;
		s = s + 1;
	}
	*s = (char) 0;
}
#endif

int rtrim(char *s) {
	while (*s) s = s + 1;
	*(s - 1) = (char) 0;
}

int bubble(char *s) {
	int i = 0;
	int j = 0;
	char tmp = 0;

	/* find string length */
	while (*(s + i)) i = i + 1;

	/* sort the string */
	while (i > 1) {
		j = 0;
		while (j < (i - 1)) {
			if (*(s+j) > *(s+j+1)) {
				tmp = *(s+j+1);
				*(s+j+1) = *(s+j);
				*(s+j) = tmp;
			}
			j = j + 1;
		}
		i = i - 1;
	}
}

int main() {
	char data[80];
	puts ("Input:  ");
	gets (data);
	rtrim (data);  /* remove trailing newline */
	bubble (data);
	puts ("Output: ");
	puts (data);
	putchar (10);
}
