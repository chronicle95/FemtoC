/* Testing runtime */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char debug_mode = 0;

typedef unsigned short TYPE;

#define BUF_SZ	65535 
#define SF_SZ	64
#define ID_SZ	24
#define LBL_CNT	1000

#define OP_NOP    0
#define OP_PUSHL  1
#define OP_PUSHI  2
#define OP_PUSH   3
#define OP_PUSHSF 4
#define OP_POPI   5
#define OP_NOT    6
#define OP_INV    7
#define OP_ADD    8
#define OP_SUB    9
#define OP_MUL    10
#define OP_DIV    11
#define OP_MOD    12
#define OP_RET    13
#define OP_CALL   14
#define OP_JUMP   15
#define OP_NZJUMP 16
#define OP_CMPEQ  17
#define OP_CMPNE  18
#define OP_CMPGT  19
#define OP_CMPGE  20
#define OP_CMPLT  21
#define OP_CMPLE  22
#define OP_AND    23
#define OP_OR     24
#define OP_DUP    25
#define OP_DROP   26
#define OP_SWAP   27
#define OP_INPUT  28
#define OP_OUTPUT 29
#define OP_HALT   255

#define IGN_WHITESPACE(fd)	while (c == ' ' || c == '\t') c = fgetc(fd)
#define IS_NUMBER(c)	(c >= '0' && c <= '9')
#define IS_ALLOWED(c)	((c >= 'A' && c <= 'Z')\
						|| (c >= 'a' && c <= 'z')\
						|| IS_NUMBER(c) || (c == '_'))

typedef struct asm_label
{
	char text[ID_SZ];
	TYPE addr;

} asm_label;

void show_info()
{
	puts ("Development runtime for FemtoC");
	puts ("Usage:");
	puts ("./rr [--debug] <file>");
}

int assemble_file(const char *file_name, TYPE *buf)
{
	asm_label l[LBL_CNT];
	char tmp[ID_SZ];
	int lbl_idx = 0;
	int buf_idx = 0;
	int tmp_idx = 0;
	char c;
	FILE *f = fopen(file_name, "r");

	if (debug_mode)
	{
		puts ("Assembling...");
	}

	if (!f)
	{
		printf ("error opening file: %s\n", file_name);
		return 0;
	}

	/* First phase */
	for (c = fgetc(f); !feof(f); c = fgetc(f))
	{
		/* ignore whitespace at the beginning of the line */
		IGN_WHITESPACE(f);

		/* if the line is empty, proceed to the next one */
		if (c == '\n') continue;

		/* read preprocessor */
		if (c == '.')
		{
			for (tmp_idx = 0, c = fgetc(f); IS_ALLOWED(c); tmp[tmp_idx++] = c, c = fgetc(f));
			tmp[tmp_idx] = 0;
			if (!strcmp (tmp, "byte"))
			{
				while (c != '\n')
				{
					IGN_WHITESPACE(f);
					if (IS_NUMBER (c))
					{
						for (tmp_idx = 0; IS_NUMBER(c); tmp[tmp_idx++] = c, c = fgetc(f));
						tmp[tmp_idx] = 0;
						buf[buf_idx++] = atoi (tmp);
					}
					else
					{
						puts ("error: number expected");
						return 0;
					}
				}
			}
			else if (!strcmp (tmp, "zero"))
			{
				int i;
				IGN_WHITESPACE(f);
				if (IS_NUMBER (c))
				{
					for (tmp_idx = 0; IS_NUMBER (c); tmp[tmp_idx++] = c, c = fgetc(f));
					tmp[tmp_idx] = 0;
					i = atoi (tmp);
					while (i > 0)
					{
						buf[buf_idx++] = 0;
						i--;
					}
				}
				else
				{
					puts ("error: number expected");
					return 0;
				}
			}
			continue;
		}
		else if (IS_ALLOWED(c))
		{
			/* read opcode or label */
			for (tmp_idx = 0; IS_ALLOWED(c); tmp[tmp_idx++] = c, c = fgetc(f));
			tmp[tmp_idx] = 0;
		}

		if (c == ':' && tmp_idx != 0)
		{
			int i;
			/* process the label */
			for (i = 0; i < lbl_idx; i++)
			{
				if (!strcmp (l[i].text, tmp)) break;
			}
			if (i == lbl_idx)
			{
				lbl_idx++;
				if (lbl_idx == LBL_CNT)
				{
					puts("error: too many labels!");
					return 0;
				}
				strcpy (l[i].text, tmp);
			}
			l[i].addr = buf_idx;
			tmp_idx = 0;
		}
		else if (tmp_idx != 0)
		{
			/* process opcode */
			if (!strcmp (tmp, "nop")) buf[buf_idx++] = OP_NOP;
			else if (!strcmp (tmp, "add")) buf[buf_idx++] = OP_ADD;
			else if (!strcmp (tmp, "sub")) buf[buf_idx++] = OP_SUB;
			else if (!strcmp (tmp, "mul")) buf[buf_idx++] = OP_MUL;
			else if (!strcmp (tmp, "div")) buf[buf_idx++] = OP_DIV;
			else if (!strcmp (tmp, "mod")) buf[buf_idx++] = OP_MOD;
			else if (!strcmp (tmp, "inv")) buf[buf_idx++] = OP_INV;
			else if (!strcmp (tmp, "not")) buf[buf_idx++] = OP_NOT;
			else if (!strcmp (tmp, "dup")) buf[buf_idx++] = OP_DUP;
			else if (!strcmp (tmp, "drop")) buf[buf_idx++] = OP_DROP;
			else if (!strcmp (tmp, "swap")) buf[buf_idx++] = OP_SWAP;
			else if (!strcmp (tmp, "jump")) buf[buf_idx++] = OP_JUMP;
			else if (!strcmp (tmp, "nzjump")) buf[buf_idx++] = OP_NZJUMP;
			else if (!strcmp (tmp, "cmpeq")) buf[buf_idx++] = OP_CMPEQ;
			else if (!strcmp (tmp, "cmpne")) buf[buf_idx++] = OP_CMPNE;
			else if (!strcmp (tmp, "cmpgt")) buf[buf_idx++] = OP_CMPGT;
			else if (!strcmp (tmp, "cmplt")) buf[buf_idx++] = OP_CMPLT;
			else if (!strcmp (tmp, "cmpge")) buf[buf_idx++] = OP_CMPGE;
			else if (!strcmp (tmp, "cmple")) buf[buf_idx++] = OP_CMPLE;
			else if (!strcmp (tmp, "and")) buf[buf_idx++] = OP_AND;
			else if (!strcmp (tmp, "or")) buf[buf_idx++] = OP_OR;
			else if (!strcmp (tmp, "ret")) buf[buf_idx++] = OP_RET;
			else if (!strcmp (tmp, "call")) buf[buf_idx++] = OP_CALL;
			else if (!strcmp (tmp, "pushi")) buf[buf_idx++] = OP_PUSHI;
			else if (!strcmp (tmp, "pushsf")) buf[buf_idx++] = OP_PUSHSF;
			else if (!strcmp (tmp, "popi")) buf[buf_idx++] = OP_POPI;
			else if (!strcmp (tmp, "halt")) buf[buf_idx++] = OP_HALT;
			else if (!strcmp (tmp, "input")) buf[buf_idx++] = OP_INPUT;
			else if (!strcmp (tmp, "output")) buf[buf_idx++] = OP_OUTPUT;
			else if (!strcmp (tmp, "pushl"))
			{
				int i;
				IGN_WHITESPACE (f);
				if (!IS_ALLOWED (c))
				{
					puts ("error: label expected with PUSHL");
					return 0;
				}
				for (i = 0; IS_ALLOWED (c); tmp[i++] = c, c = fgetc(f));
				tmp[i] = 0;
				for (i = 0; i < lbl_idx; i++)
				{
					if (!strcmp (l[i].text, tmp)) break;
				}
				if (i == lbl_idx)
				{
					lbl_idx++;
					if (lbl_idx == LBL_CNT)
					{
						puts("error: too many labels!");
						return 0;
					}
					strcpy (l[i].text, tmp);
					l[i].addr = -1;
				}
				buf[buf_idx++] = OP_PUSHL;
				buf[buf_idx++] = i;
			}
			else if (!strcmp (tmp, "push"))
			{
				IGN_WHITESPACE (f);
				for (tmp_idx = 0; IS_NUMBER (c); tmp[tmp_idx++] = c, c = fgetc(f));
				tmp[tmp_idx] = 0;
				buf[buf_idx++] = OP_PUSH;
				buf[buf_idx++] = atoi (tmp);
			}
			tmp_idx = 0;
		}

		/* ignore remaining whitespace and comments */
		IGN_WHITESPACE(f);
		if (c == ';') while (c != '\n') c = fgetc(f);
	}

	/* Second phase */
	for (buf_idx = 0; buf_idx < BUF_SZ; buf_idx++)
	{
		if (buf[buf_idx] == OP_PUSH) buf_idx++;
		else if (buf[buf_idx] == OP_PUSHL)
		{
			buf[buf_idx] = OP_PUSH;
			buf_idx++;
			if (l[buf[buf_idx]].addr == -1)
			{
				printf ("error: unresolved label: %s\n", l[buf[buf_idx]].text);
				return 0;
			}
			buf[buf_idx] = l[buf[buf_idx]].addr;
		}
	}

	fclose(f);

	return 1;
}

void execute_binary(TYPE *m)
{
	TYPE sf[SF_SZ];
	TYPE sfh = SF_SZ;
	TYPE sh = BUF_SZ;
	TYPE cp = 0;

	if (debug_mode)
	{
		puts ("Executing binary...");
	}

	for (;; cp++)
	{
		if (debug_mode)
		{
			printf ("@%06d OP_%-4d ", cp, m[cp]);
		}
#define NEED_STACK(must_have) do {\
			if (debug_mode && ((BUF_SZ - sh) < must_have)) {\
				puts ("\nRuntime error: stack underflow");\
				return;\
			} } while(0)

		switch (m[cp])
		{
			case OP_NOP:
				/* do nothing */
				break;
			case OP_ADD: NEED_STACK(2);
				m[sh + 1] += m[sh];
				sh++;
				break;
			case OP_SUB: NEED_STACK(2);
				m[sh + 1] -= m[sh];
				sh++;
				break;
			case OP_MUL: NEED_STACK(2);
				m[sh + 1] *= m[sh];
				sh++;
				break;
			case OP_DIV: NEED_STACK(2);
				m[sh + 1] /= m[sh];
				sh++;
				break;
			case OP_MOD: NEED_STACK(2);
				m[sh + 1] %= m[sh];
				sh++;
				break;
			case OP_INV: NEED_STACK(1);
				m[sh] = ~m[sh];
				break;
			case OP_NOT: NEED_STACK(1);
				m[sh] = !m[sh];
				break;
			case OP_DUP: NEED_STACK(1);
				sh--;
				m[sh] = m[sh + 1];
				break;
			case OP_DROP: NEED_STACK(1);
				sh++;
				break;
			case OP_SWAP: NEED_STACK(2);
				m[sh] ^= m[sh + 1];
				m[sh + 1] ^= m[sh];
				m[sh] ^= m[sh + 1];
				break;
			case OP_PUSHI: NEED_STACK(1);
				m[sh] = m[m[sh]];
				break;
			case OP_PUSHSF:
				if (sfh != SF_SZ)
				{
					sh--;
					m[sh] = sf[sfh];
				}
				break;
			case OP_POPI: NEED_STACK(2);
				m[m[sh + 1]] = m[sh];
				sh += 2;
				break;
			case OP_PUSH:
				sh--;
				cp++;
				m[sh] = m[cp];
				if (debug_mode)
				{
					printf (" Argument: %4d", m[cp]);
				}
				break;
			case OP_NZJUMP: NEED_STACK(2);
				if (m[sh + 1])
				{
					m[sh + 1] = m[sh];
					sh++;
				}
				else
				{
					sh += 2;
					break;
				}
				/* fall through */
			case OP_JUMP: NEED_STACK(1); 
				cp = m[sh] - 1;
				sh++;
				break;
			case OP_CMPEQ: NEED_STACK(2);
				m[sh + 1] = (m[sh] == m[sh + 1]);
				sh++;
				break;
			case OP_CMPNE: NEED_STACK(2);
				m[sh + 1] = (m[sh] != m[sh + 1]);
				sh++;
				break;
			case OP_CMPLT: NEED_STACK(2);
				m[sh + 1] = (m[sh] > m[sh + 1]);
				sh++;
				break;
			case OP_CMPGT: NEED_STACK(2);
				m[sh + 1] = (m[sh] < m[sh + 1]);
				sh++;
				break;
			case OP_CMPGE: NEED_STACK(2);
				m[sh + 1] = (m[sh] <= m[sh + 1]);
				sh++;
				break;
			case OP_CMPLE: NEED_STACK(2);
				m[sh + 1] = (m[sh] >= m[sh + 1]);
				sh++;
				break;
			case OP_AND: NEED_STACK(2);
				m[sh + 1] = (m[sh] && m[sh + 1]);
				sh++;
				break;
			case OP_OR: NEED_STACK(2);
				m[sh + 1] = (m[sh] || m[sh + 1]);
				sh++;
				break;
			case OP_INPUT:
				sh--;
				if (debug_mode)
				{
					printf (" Inp.(int): ");
					scanf ("%d", &m[sh]);
				}
				else
				{
					m[sh] = getchar ();
				}
				break;
			case OP_OUTPUT: NEED_STACK(1);
				if (debug_mode)
				{
					printf (" Output  : %4d", m[sh]);
				}
				else
				{
					putchar ((char) m[sh]);
				}
				sh++;
				break;
			case OP_CALL: NEED_STACK(1);
				cp ^= m[sh];
				m[sh] ^= cp;
				cp ^= m[sh];
				sf[--sfh] = sh;
				cp--;
				break;
			case OP_RET: NEED_STACK(1);
				if (sfh != SF_SZ)
				{
					sh = sf[sfh++];
					cp = m[sh++];
				}
				break;
			case OP_HALT:
				return;
		}
#undef STACK
		if (debug_mode)
		{
			printf ("\n");
			usleep(100000);
		}
	}
}

void execute_file(const char *file_name)
{
	TYPE *buffer;
	buffer = (TYPE *) malloc (BUF_SZ * sizeof (TYPE));
	if (assemble_file (file_name, buffer))
	{
		execute_binary (buffer);
	}
	free (buffer);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		goto info_exit;
	}
	if (!strcmp (argv[1], "--debug"))
	{
		debug_mode = 1;
	}
	execute_file (argv[1 + debug_mode]);
	if (debug_mode)
	{
		puts ("\nDone!");
	}
	return 0;
info_exit:
	show_info();
	return 1;
}
