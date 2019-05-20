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

typedef struct asm_opcode
{
	int code;
	char op[ID_SZ];
	struct asm_opcode *next;
} asm_opcode;

asm_opcode *first = NULL;

void append_opcode(int code, const char *op)
{
	asm_opcode *p = first;
	while (p && p->next) p = p->next;
	if (!p)
	{
		first = (asm_opcode*) malloc (sizeof(asm_opcode));
		strcpy (first->op, op);
		first->code = code;
		first->next = NULL;
	}
	else
	{
		p->next = (asm_opcode*) malloc (sizeof(asm_opcode));
		strcpy (p->next->op, op);
		p->next->code = code;
		p->next->next = NULL;
	}
}

const char *get_opcode (int code)
{
	asm_opcode *p = first;
	while (p)
	{
		if (p->code == code) break;
		p = p->next;
	}
	if (!p) return NULL;
	return p->op;
}

int get_opcode_i (const char *op)
{
	asm_opcode *p = first;
	while (p)
	{
		if (!strcmp (p->op, op)) break;
		p = p->next;
	}
	if (!p) return -1;
	return p->code;
}

void init_opcodes()
{
	append_opcode (OP_NOP, "nop");
	append_opcode (OP_ADD, "add");
	append_opcode (OP_SUB, "sub");
	append_opcode (OP_DIV, "div");
	append_opcode (OP_MUL, "mul");
	append_opcode (OP_MOD, "mod");
	append_opcode (OP_INV, "inv");
	append_opcode (OP_NOT, "not");
	append_opcode (OP_AND, "and");
	append_opcode (OP_OR, "or");
	append_opcode (OP_DUP, "dup");
	append_opcode (OP_DROP, "drop");
	append_opcode (OP_SWAP, "swap");
	append_opcode (OP_PUSH, "push");
	append_opcode (OP_PUSHL, "pushl");
	append_opcode (OP_CMPEQ, "cmpeq");
	append_opcode (OP_CMPNE, "cmpne");
	append_opcode (OP_CMPLT, "cmplt");
	append_opcode (OP_CMPGT, "cmpgt");
	append_opcode (OP_CMPLE, "cmple");
	append_opcode (OP_CMPGE, "cmpge");
	append_opcode (OP_JUMP, "jump");
	append_opcode (OP_NZJUMP, "nzjump");
	append_opcode (OP_PUSHI, "pushi");
	append_opcode (OP_PUSHSF, "pushsf");
	append_opcode (OP_POPI, "popi");
	append_opcode (OP_CALL, "call");
	append_opcode (OP_RET, "ret");
	append_opcode (OP_INPUT, "input");
	append_opcode (OP_OUTPUT, "output");
	append_opcode (OP_HALT, "halt");
}

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
			int op = get_opcode_i (tmp);
			if (op == OP_PUSHL)
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
			else if (op == OP_PUSH)
			{
				IGN_WHITESPACE (f);
				for (tmp_idx = 0; IS_NUMBER (c); tmp[tmp_idx++] = c, c = fgetc(f));
				tmp[tmp_idx] = 0;
				buf[buf_idx++] = OP_PUSH;
				buf[buf_idx++] = atoi (tmp);
			}
			else if (op != -1)
			{
				buf[buf_idx++] = op;
			}
			else
			{
				printf ("error: bad opcode: %s\n", tmp);
				return 0;
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
			if (l[buf[buf_idx]].addr == (TYPE)-1)
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
			/* Print top few values of the stack */
			int j, k, a_frame;
			for (j=7; j>=0; j--)
			{
				/* Check if current memory cell is a frame */
				a_frame = 0;
				for (k = sfh; k<SF_SZ; k++)
				{
					if ((sh + j) == sf[k])
					{
						a_frame = 1;
						break;
					}
				}

				if (sh + j < BUF_SZ)
				{
					TYPE n = m[sh + j];
					for (k=0; k<sizeof(TYPE); k++)
					{
						if (a_frame) printf ("\x1b[4m");
						printf ("%02x", (unsigned char)*((char*)&n + k));
						if (a_frame) printf ("\x1b[0m");
					}
				}
				else
				{
					for (k=0; k<sizeof(TYPE); k++)
					{
						printf ("..");
					}
				}
				putchar (' ');
			}
			printf ("@%06d %-6s ", cp, get_opcode(m[cp]));
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
					int d;
					printf (" Inp.(int): ");
					scanf ("%d", &d);
					m[sh] = d;
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
#undef NEED_STACK
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
	init_opcodes ();
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
