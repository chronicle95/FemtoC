/* Testing runtime */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short TYPE;

#define BUF_SZ  65535 
#define ID_SZ   24
#define LBL_CNT 1000

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
#define OP_DUP    19
#define OP_DROP   20
#define OP_SWAP   21
#define OP_INPUT  22
#define OP_OUTPUT 23
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
	puts ("./rr <file>");
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
			/* TODO */
			continue;
		}
		else if (IS_ALLOWED(c))
		{
			tmp_idx = 0;
			/* read opcode or label */
			while (IS_ALLOWED(c))
			{
				tmp[tmp_idx++] = c;
				c = fgetc(f);
			}
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
	TYPE sh = BUF_SZ - 1;
	TYPE cp = 0;

	for (;; cp++)
	{
		switch (m[cp])
		{
			case OP_NOP:
				/* do nothing */
				break;
			case OP_ADD:
				m[sh + 1] += m[sh];
				sh++;
				break;
			case OP_SUB:
				m[sh + 1] -= m[sh];
				sh++;
				break;
			case OP_MUL:
				m[sh + 1] *= m[sh];
				sh++;
				break;
			case OP_DIV:
				m[sh + 1] /= m[sh];
				sh++;
				break;
			case OP_MOD:
				m[sh + 1] %= m[sh];
				sh++;
				break;
			case OP_INV:
				m[sh] = ~m[sh];
				break;
			case OP_NOT:
				m[sh] = !m[sh];
				break;
			case OP_DUP:
				sh--;
				m[sh] = m[sh + 1];
				break;
			case OP_DROP:
				sh++;
				break;
			case OP_PUSHI:
				m[sh] = m[m[sh]];
				break;
			case OP_POPI:
				m[m[sh + 1]] = m[sh];
				sh += 2;
				break;
			case OP_PUSH:
				sh--;
				cp++;
				m[sh] = m[cp];
				break;
			case OP_NZJUMP:
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
			case OP_JUMP:
				cp = m[sh];
				sh++;
				break;
			case OP_CMPEQ:
				m[sh + 1] = (m[sh] == m[sh + 1]);
				sh++;
				break;
			case OP_CMPNE:
				m[sh + 1] = (m[sh] != m[sh + 1]);
				sh++;
				break;
			case OP_INPUT:
				sh--;
				m[sh] = getchar ();
				break;
			case OP_OUTPUT:
				putchar ((char) m[sh]);
				sh++;
			case OP_HALT:
				return;
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
	if (argc != 2)
	{
		show_info ();
		return 1;
	}
	execute_file (argv[1]);
	return 0;
}
