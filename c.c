/*
 * This file is part of the FemtoC project (https://github.com/chronicle95).
 * Copyright (c) 2017-2019 Artem Bondarenko.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

/* Explicit declarations */
int parse_statement();
int parse_loop_for();
int parse_loop_while();
int parse_conditional();

/* Global variables: Limits */
int ID_SZ = 24;     /* maximum identifier length */
int SRC_SZ = 32000; /* up to ~2000 lines of C source code */
int OUT_SZ = 65000; /* up to ~5500 lines of assembly output */
int LOC_SZ = 800;   /* up to 32 local variables */
int GBL_SZ = 6400;  /* up to 256 global identifiers (f + v) */
int ARG_SZ = 200;   /* up to 8 arguments per function */

char* src_p = 0; /* source code pointer
		    points to current location */
char* out_p = 0; /* output pointer
		    points to current location */
char* loc_p = 0; /* function locals list.
		    points to the beginning.
		    record goes as follow:

		    f-var0 f-var1 f-var2 ... */
char* arg_p = 0; /* function arguments list.
		    points to the beginning.
		    record goes as follow:

		    f-arg0 f-arg1 f-arg2 ... */
char* gbl_p = 0; /* global variable list.
		    points to the beginning.
		    record goes as follow:

		    var0 var1 ... */
int lbl_cnt = 0;   /* Label counter. Used for temporary labels */
char* lbl_sta = 0; /* Nearest loop start label */
char* lbl_end = 0; /* Nearest loop end label */

/* Utility functions */

int copy_memory(char *dst, char *src, int size)
{
	while (size)
	{
		*dst = *src;
		dst = dst + 1;
		src = src + 1;
		size = size - 1;
	}
	return 0;
}

int clear_memory(char *p, int size)
{
	while (size > 0)
	{
		*p = 0;
		p = p + 1;
		size = size - 1;
	}
	return 0;
}

int gen_label(char *dst)
{
	int n = lbl_cnt;
	copy_memory (dst, "_L_", 3);
	dst = dst + 3;
	if (n == 0)
	{
		*dst = 'a';
		dst = dst + 1;
	}
	else
	{
		while (n > 0)
		{
			*dst = 'a' + (n % 26);
			dst = dst + 1;
			n = n / 26;
		}
	}
	*dst = 0;
	lbl_cnt = lbl_cnt + 1;
	return 1;
}

int strcomp(char* a, char* b)
{
	if ((a == 0) && (b == 0))
	{
		return 1;
	}
	if ((a == 0) || (b == 0))
	{
		return 0;
	}
	while (*a && *b)
	{
		if (*a != *b) 
		{
			return 0;
		}
		a = a + 1;
		b = b + 1;
	}
	if (*a || *b)
	{
		return 0;
	}
	return 1;
}

int write_chr(char c)
{
	*out_p = c;
	out_p = out_p + 1;
	*out_p = 0;
	return 1;
}

int write_str(char *s)
{
	while (*s)
	{
		write_chr (*s);
		s = s + 1;
	}
	return 1;
}

int write_strln(char *s)
{
	write_str (s);
	write_chr (10);
	return 1;
}

int error_log(char *s)
{
	write_str (";; [ERROR]: ");
	write_strln (s);
	return 0;
}

int write_num(int n)
{
	char buf[10];
	int i = 0;

	if (n == 0)
	{
		write_str ("0");
		return 1;
	}

	while (n > 0)
	{
		*(buf + i) = '0' + (n % 10);
		n = n / 10;
		i = i + 1;
	}

	i = i - 1;
	while (i > 0)
	{
		write_chr (*(buf + i));
		i = i - 1;
	}
	write_chr (*buf);

	return 1;
}

int write_numln(int n)
{
	write_num (n);
	write_chr (10);
	return 1;
}

int store_var(char *ptr, char *s)
{
	char *fp = ptr;
	/* look for the end */
	while (*fp)
	{
		fp = fp + 1;
	}
	/* append a word */
	while (*s)
	{
		*fp = *s;
		s = s + 1;
		fp = fp + 1;
	}
	/* null-terminate */
	*fp = ' ';
	fp = fp + 1;
	*fp = 0;
	return 1;
}

int find_var(char *ptr, char *s, int *i)
{
	/* TODO: Imrove search function */
	char *sp = s;
	char *fp = ptr;
	*i = 0;

	while (*fp)
	{
		if (*fp == *sp)
		{
			sp = sp + 1;
		}
		else
		{
			sp = s;
			while (*fp != ' ')
			{
				fp = fp + 1;
			}
		}

		if (*sp == 0)
		{
			return 1;
		}

		if (*fp == ' ')
		{
			*i = *i + 1;
		}

		fp = fp + 1;
	}

	*i = -1;

	return 0;
}

/* Character test functions */

int is_space(char c)
{
	return ((c == ' ') || (c == 10)
			|| (c == 13)  || (c == 9));
}

int is_digit(char c)
{
	return ((c >= '0') && (c <= '9'));
}

int is_id0(char c)
{
	return ((c == '_')
			|| ((c >= 'A') && (c <= 'Z'))
			|| ((c >= 'a') && (c <= 'z')));
}

int is_id(char c)
{
	return is_id0 (c)
		|| is_digit (c);
}

/* Read functions */

int read_space()
{
	while (1)
	{
		/* Ignore comments */
		if ((*src_p == '/') && (*(src_p+1) == '*'))
		{
			while (!((*src_p == '*') && (*(src_p+1) == '/')))
			{
				src_p = src_p + 1;
			}
			src_p = src_p + 2;
		}

		/* Ignore spaces */
		if (is_space (*src_p))
		{
			src_p = src_p + 1;
		}
		/* Ignore preprocessor
		 * TBD: implement include
		 */
		else if (*src_p == '#')
		{
			while (*src_p && (*src_p != 10))
			{
				src_p = src_p + 1;
			}
		}
		else
		{
			break;
		}
	}
	return 1;
}

int read_sym(char exp)
{
	read_space ();
	if (*src_p == exp)
	{
		src_p = src_p + 1;
		return 1;
	}
	return 0;
}

int read_sym_s(char *exp_s)
{
	int count = 0;

	read_space ();

	while (*exp_s && (*src_p == *exp_s))
	{
		src_p = src_p + 1;
		exp_s = exp_s + 1;
		count = count + 1;
	}

	if (*exp_s || is_id (*src_p))
	{
		src_p = src_p - count;
		return 0;
	}

	return 1;
}

int unread_sym()
{
	src_p = src_p - 1;
	return 1;
}

int read_str_const()
{
	while (*src_p && (*src_p != '"'))
	{
		write_str (" ");
		write_num (*src_p);
		src_p = src_p + 1;
	}
	src_p = src_p + 1;
	return 1;
}

int read_id(char* dst)
{
	char* dp = dst;

	read_space ();
	if (!is_id0 (*src_p))
	{
		return 0;
	}
	while (is_id (*src_p))
	{
		*dp = *src_p;
		dp = dp + 1;
		src_p = src_p + 1;
	}
	*dp = 0;

	/* Types are ignored */
	if (strcomp (dst, "int") || strcomp (dst, "char"))
	{
		/* Bypass pointer asterisks too */
		while (read_sym ('*'));
		/* Read the actual identifier */
		return read_id (dst);
	}

	return 1;
}

int read_number(char* dst)
{
	read_space ();
	if (!is_digit (*src_p))
	{
		return 0;
	}
	while (is_digit (*src_p))
	{
		*dst = *src_p;
		dst = dst + 1;
		src_p = src_p + 1;
	}
	*dst = 0;
	return 1;
}

/* Parse and process
 * functions */

int parse_expr();
int parse_invoke(char *name)
{
	int argcnt = 0;
	int n = 0;
	int len = 0;
	char *argpos[ARG_SZ / (ID_SZ + 1) + 1];

	/* use argpos to locate where the output goes */
	*(argpos + argcnt) = out_p;

	while (1)
	{
		if (parse_expr ())
		{
			argcnt = argcnt + 1;
			*(argpos + argcnt) = out_p;
			continue;
		}
		else if (read_sym (','))
		{
			continue;
		}
		else if (read_sym (')'))
		{
			break;
		}
		return 0;
	}

	/* Now that we have all our arguments prepared,
	 * reverse them so the addressing is right */
	if (argcnt)
	{
		if (argcnt == 2)
		{
			write_strln ("  swap");
		}
		else
		{
			len = *(argpos + argcnt) - *(argpos);
			copy_memory (*(argpos + argcnt), *(argpos), len);
			n = 0;
			while (n < argcnt)
			{
				copy_memory ((len - (*(argpos + n + 1) - *(argpos))) + *(argpos),
					*(argpos + n) + len,
					*(argpos + n + 1) - *(argpos + n));
				n = n + 1;
			}
			clear_memory (*(argpos) + len, len);
		}
	}

	/* Call the subroutine */
	write_str ("  pushl ");
	write_strln (name);
	write_strln ("  call");

	/* Drop the arguments */
	n = 0;
	while (n < argcnt)
	{
		write_strln ("  drop");
		n = n + 1;
	}

	return 1;
}

int parse_operand()
{
	char buf[ID_SZ];
	char lbl[ID_SZ];
	int idx = 0;
	if (read_sym ('!'))
	{
		if (parse_operand())
		{
			write_strln ("  not");
			return 1;
		}
	}
	else if (read_sym ('&'))
	{
		if (!read_id (buf))
		{
			return 0;
		}
		if (find_var (loc_p, buf, &idx))
		{
			write_strln ("  pushsf");
			write_str ("  push ");
			write_numln (idx);
			write_strln ("  sub");
		}
		else if (find_var (arg_p, buf, &idx))
		{
			write_strln ("  pushsf");
			write_str ("  push ");
			write_numln (idx);
			write_strln ("  add");
		}
		else if (find_var (gbl_p, buf, &idx))
		{
			write_str ("  pushl ");
			write_strln (buf);
		}
		else
		{
			error_log ("undeclared identifier");
			return 0;
		}
		return 1;
	}
	else if (read_sym ('~'))
	{
		if (parse_operand())
		{
			write_strln ("  inv");
			return 1;
		}
	}
	else if (read_sym ('*'))
	{
		if (parse_operand())
		{
			write_strln ("  pushi");
			return 1;
		}
	}
	else if (read_sym ('('))
	{
		if (parse_expr ())
		{
			return read_sym (')');
		}
	}
	else if (read_sym ('"'))
	{
		gen_label (buf);
		gen_label (lbl);
		write_str ("  pushl ");
		write_strln (buf);
		write_strln ("  jump");
		write_str (lbl);
		write_strln (":");
		write_str (" .word");
		read_str_const ();
		write_strln (" 0");
		write_str (buf);
		write_strln (":");
		write_str ("  pushl ");
		write_strln (lbl);
		return 1;
	}
	else if (read_sym (39))
	{
		write_str ("  push ");
		write_numln (*src_p);
		src_p = src_p + 1;
		if (!read_sym (39))
		{
			return 0;
		}
		return 1;
	}
	else if (read_number (buf))
	{
		write_str ("  push ");
		write_strln (buf);
		return 1;
	}
	else if (read_id (buf))
	{
		if (read_sym ('('))
		{
			if (!parse_invoke (buf))
			{
				return 0;
			}
			write_strln ("  pushl __retval");
			write_strln ("  pushi");
		}
		else
		{
			if (find_var (loc_p, buf, &idx))
			{
				write_strln ("  pushsf");
				write_str ("  push ");
				write_numln (idx);
				write_strln ("  sub");
				write_strln ("  pushi");
			}
			else if (find_var (arg_p, buf, &idx))
			{
				write_strln ("  pushsf");
				write_str ("  push ");
				write_numln (idx);
				write_strln ("  add");
				write_strln ("  pushi");
			}
			else if (find_var (gbl_p, buf, &idx))
			{
				write_str ("  pushl ");
				write_strln (buf);
				write_strln ("  pushi");
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}
	else if (read_sym ('-'))
	{
		if (parse_operand ())
		{
			write_strln ("  inv");
			write_strln ("  push 1");
			write_strln ("  add");
			return 1;
		}
	}
	return 0;
}

int parse_expr()
{
	if (!parse_operand ())
	{
		return 0;
	}
	/* Any closing brackets, commas and semicolons
	 * are considered the end of expression */
	while (!read_sym (',')
			&& !read_sym (';')
			&& !read_sym (')')
			&& !read_sym (']'))
	{

		if (read_sym ('+'))
		{
			parse_operand ();
			write_strln ("  add");
		}
		else if (read_sym ('-'))
		{
			parse_operand ();
			write_strln ("  sub");
		}
		else if (read_sym ('*'))
		{
			parse_operand ();
			write_strln ("  mul");
		}
		else if (read_sym ('/'))
		{
			parse_operand ();
			write_strln ("  div");
		}
		else if (read_sym ('%'))
		{
			parse_operand ();
			write_strln ("  mod");
		}
		else if (read_sym ('='))
		{
			if (read_sym ('='))
			{
				parse_operand ();
				write_strln ("  cmpeq");
			}
			else
			{
				return 0;
			}
		}
		else if (read_sym ('<'))
		{
			if (read_sym ('='))
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  cmple");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  cmplt");
			}
		}
		else if (read_sym ('>'))
		{
			if (read_sym ('='))
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  cmpge");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  cmpgt");
			}
		}
		else if (read_sym ('!'))
		{
			if (!read_sym ('='))
			{
				return 0;
			}
			if (!parse_operand ())
			{
				return 0;
			}
			write_strln ("  cmpne");
		}
		else if (read_sym ('&'))
		{
			if (read_sym ('&'))
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  not");
				write_strln ("  swap");
				write_strln ("  not");
				write_strln ("  or");
				write_strln ("  not");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  and");
			}
		}
		else if (read_sym ('|'))
		{
			if (read_sym ('|'))
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  not");
				write_strln ("  swap");
				write_strln ("  not");
				write_strln ("  and");
				write_strln ("  not");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("  or");
			}
		}
		else
		{
			return 0;
		}
	}
	/* Parent rules should take care of the terminator */
	unread_sym ();
	return 1;
}

int parse_keyword_block()
{
	if (read_sym_s ("if"))
	{
		if (!parse_conditional ())
		{
			return 0;
		}
	}
	else if (read_sym_s ("while"))
	{
		if (!parse_loop_while ())
		{
			return 0;
		}
	}
	else if (read_sym_s ("for"))
	{
		if (!parse_loop_for ())
		{
			return 0;
		}
	}
	else if (read_sym_s ("asm"))
	{
		if (!read_sym ('{'))
		{
			error_log ("`{` expected");
			return 0;
		}
		write_strln(";; ASM {");
		while (!read_sym ('}'))
		{
			read_space ();
			write_str ("  ");
			while ((*src_p != 10) && (*src_p != '}'))
			{
				write_chr (*src_p);
				src_p = src_p + 1;
			}
			write_chr (10);
		}
		write_strln(";; } ASM");
	}
	else
	{
		return 0;
	}
	return 1;
}

int parse_block()
{
	if (!read_sym ('{'))
	{
		if (!parse_keyword_block ())
		{
			if (!parse_statement ())
			{
				return 0;
			}
			if (!read_sym (';'))
			{
				return 0;
			}
		}
	}
	else
	{
		while (!read_sym ('}'))
		{
			if (!parse_keyword_block ())
			{
				if (!parse_statement ())
				{
					return 0;
				}
				if (!read_sym (';'))
				{
					return 0;
				}
			}
		}
	}
	return 1;
}

int parse_conditional()
{
	char lbl[ID_SZ];

	gen_label (lbl);

	if (!read_sym ('('))
	{
		return 0;
	}

	if (!parse_expr ())
	{
		return 0;
	}

	if (!read_sym (')'))
	{
		return 0;
	}

	write_strln ("  dup");
	write_strln ("  not");
	write_str ("  pushl ");
	write_strln (lbl);
	write_strln ("  nzjump");

	if (!read_sym (';'))
	{
		if (!parse_block ())
		{
			return 0;
		}
	}

	if (read_sym_s ("else"))
	{
		write_str (lbl);
		write_strln (":");

		gen_label (lbl);
		write_str ("  pushl ");
		write_strln (lbl);
		write_strln ("  nzjump");

		if (!read_sym (';'))
		{
			if (!parse_block ())
			{
				return 0;
			}
		}

		write_str (lbl);
		write_strln (":");
	}
	else
	{
		write_str (lbl);
		write_strln (":");
		write_strln ("  drop");
	}

	return 1;
}

int parse_loop_for()
{
	char lbl1[ID_SZ];
	char lbl2[ID_SZ];
	char lbl3[ID_SZ];
	char lbl4[ID_SZ];
	char *tmp_sta = 0;
	char *tmp_end = 0;

	gen_label (lbl1);
	gen_label (lbl2);
	gen_label (lbl3);
	gen_label (lbl4);

	/* Remember parent loop labels if any */
	tmp_sta = lbl_sta;
	tmp_end = lbl_end;
	lbl_sta = lbl1;
	lbl_end = lbl2;

	if (!read_sym ('('))
	{
		return 0;
	}

	/* First statement */
	while (!read_sym (';'))
	{
		if (!parse_statement ())
		{
			return 0;
		}
		if (read_sym (','))
		{
			continue;
		}
	}

	write_str (lbl1);
	write_strln (":");

	/* Second statement: conditional */
	if (!parse_expr ())
	{
		return 0;
	}
	if (!read_sym (';'))
	{
		return 0;
	}
	write_strln ("  not");
	write_str ("  pushl ");
	write_strln (lbl4);
	write_strln ("  nzjump");

	write_str ("  pushl ");
	write_strln (lbl3);
	write_strln ("  jump");

	write_str (lbl2);
	write_strln (":");

	/* Third statement */
	while (!read_sym (')'))
	{
		if (!parse_statement ())
		{
			return 0;
		}
		if (read_sym (','))
		{
			continue;
		}
	}

	write_str ("  pushl ");
	write_strln (lbl1);
	write_strln ("  jump");

	write_str (lbl3);
	write_strln (":");

	if (!read_sym (';'))
	{
		if (!parse_block ())
		{
			return 0;
		}
	}

	write_str ("  pushl ");
	write_strln (lbl2);
	write_strln ("  jump");
	write_str (lbl4);
	write_strln (":");

	/* Restore parent loop break label */
	lbl_sta = tmp_sta;
	lbl_end = tmp_end;

	return 1;
}

int parse_loop_while()
{
	char lbl1[ID_SZ];
	char lbl2[ID_SZ];
	char *tmp_sta = 0;
	char *tmp_end = 0;

	gen_label (lbl1);
	gen_label (lbl2);

	/* Remember parent loop labels if any */
	tmp_sta = lbl_sta;
	tmp_end = lbl_end;
	lbl_sta = lbl1;
	lbl_end = lbl2;

	if (!read_sym ('('))
	{
		return 0;
	}

	write_str (lbl1);
	write_strln (":");

	if (!parse_expr ())
	{
		return 0;
	}

	if (!read_sym (')'))
	{
		return 0;
	}

	write_strln ("  not");
	write_str ("  pushl ");
	write_strln (lbl2);
	write_strln ("  nzjump");

	if (!read_sym (';'))
	{
		if (!parse_block ())
		{
			return 0;
		}
	}

	write_str ("  pushl ");
	write_strln (lbl1);
	write_strln ("  jump");
	write_str (lbl2);
	write_strln (":");

	/* Restore parent loop break label */
	lbl_sta = tmp_sta;
	lbl_end = tmp_end;

	return 1;
}

int parse_gvar(char* name)
{
	char num[ID_SZ];
	read_space ();
	if (!read_number (num))
	{
		return 0;
	}
	if (!read_sym (';'))
	{
		return 0;
	}
	write_str (name);
	write_strln (":");
	write_str (" .word ");
	write_strln (num);
	store_var (gbl_p, name);
	return 1;
}

int parse_garr(char* name)
{
	char num[ID_SZ];
	if (!read_number (num))
	{
		return 0;
	}
	if (!read_sym (']'))
	{
		return 0;
	}
	if (!read_sym (';'))
	{
		return 0;
	}
	write_str (name);
	write_strln (":");
	write_str (" .zero ");
	write_strln (num);
	store_var (gbl_p, name);
	return 1;
}

int parse_statement()
{
	int idx = 0;
	char id[ID_SZ];
	char num[ID_SZ];

	if (read_sym ('*'))
	{
		if (!parse_operand ())
		{
			return 0;
		}
		if (!read_sym ('='))
		{
			return 0;
		}
		if (!parse_expr ())
		{
			return 0;
		}
		write_strln ("  popi");
		return 1;
	}

	/*
	 *  Keywords checked first
	 */
	else if (read_sym_s ("return"))
	{
		/* calculate return value */
		if (!parse_expr ())
		{
			return 0;
		}
		/* save it */
		write_strln ("  pushl __retval");
		write_strln ("  swap");
		write_strln ("  popi");
		/* jump to end of function */
		idx = 0;
		while (*(loc_p + idx) != ' ')
		{
			*(id + idx) = *(loc_p + idx);
			idx = idx + 1;
		}
		*(id + idx) = 0;
		write_str ("  pushl __");
		write_str (id);
		write_strln ("_end");
		write_strln("  jump");
		return 1;
	}
	else if (read_sym_s ("goto"))
	{
		if (!read_id (id))
		{
			error_log ("label expected");
			return 0;
		}
		write_str ("  pushl __");
		write_strln (id);
		write_strln ("jump");
		return 1;
	}
	else if (read_sym_s ("break"))
	{
		if (lbl_end)
		{
			write_str ("  pushl ");
			write_strln (lbl_end);
			write_strln ("  jump");
		}
		return 1;
	}
	else if (read_sym_s ("continue"))
	{
		if (lbl_sta)
		{
			write_str ("  pushl ");
			write_strln (lbl_sta);
			write_strln ("  jump");
		}
		return 1;
	}

	/* Otherwise check for identifier */
	else if (!read_id (id))
	{
		error_log ("identifier expected");
		return 0;
	}


	/* Function call */
	if (read_sym ('('))
	{
		return parse_invoke (id);
	}

	/* Simple variable assignment */
	else if (read_sym ('='))
	{
		if (!parse_expr ())
		{
			return 0;
		}
		if (find_var (loc_p, id, &idx))
		{
			write_strln ("  pushsf");
			write_str ("  push ");
			write_numln (idx);
			write_strln ("  sub");
		}
		else if (find_var (arg_p, id, &idx))
		{
			write_strln ("  pushsf");
			write_str ("  push ");
			write_numln (idx);
			write_strln ("  add");
		}
		else if (find_var (gbl_p, id, &idx))
		{
			write_str ("  pushl ");
			write_strln (id);
		}
		else
		{
			/* In a case of new allocation we need to provide
			 * space on stack for it so that we a have a place
			 * to store the value */
			write_strln ("  dup");

			/* Allocate and point */
			store_var (loc_p, id);
			find_var (loc_p, id, &idx);
			write_strln ("  pushsf");
			write_str ("  push ");
			write_numln (idx);
			write_strln ("  sub");
		}
		write_strln ("  swap");
		write_strln ("  popi");
	}

	/* Label */
	else if (read_sym (':'))
	{
		write_str ("__");
		write_str (id);
		write_strln (":");
	}

	/* Local array definition.
	 * It is not placed on stack but instead dynamically
	 * allocated from pool, only the pointer stays on
	 * stack */
	else if (read_sym ('['))
	{
		/* Reserve memory cell for array pointer */
		write_strln ("  push 0");

		/* Calculate array size and leave it on the stack */
		if (!parse_expr ())
		{
			return 0;
		}
		if (!read_sym (']'))
		{
			return 0;
		}
		write_strln ("  pushl __memp");
		write_strln ("  pushi");
		write_strln ("  dup");

		store_var (loc_p, id);
		find_var (loc_p, id, &idx);

		write_strln ("  pushsf");
		write_str ("  push ");
		write_numln (idx);
		write_strln ("  sub");

		/* Initialize the array pointer */
		write_strln ("  swap");
		write_strln ("  popi");

		/* Allocate memory for the array */
		write_strln ("  add");
		write_strln ("  pushl __memp");
		write_strln ("  swap");
		write_strln ("  popi");
	}

	else
	{
		error_log ("bad statement");
		return 0;
	}

	return 1;
}

int parse_argslist()
{
	char id[ID_SZ];

	while (1)
	{
		if (!read_id (id))
		{
			if (read_sym (')'))
			{
				unread_sym ();
				return 1;
			}
			error_log ("arguments list not closed");
			return 0;
		}

		store_var (arg_p, id);

		if (!read_sym (','))
		{
			break;
		}
	}

	return 1;
}

int parse_func(char* name)
{
	char *save = out_p;

	/* Put function name to locals and arguments lists
	 * so that the first entry index starts with 1 */
	store_var (loc_p, name);
	store_var (arg_p, name);

	/* Put function name to globals list */
	store_var (gbl_p, name);

	/* Put label */
	write_str (name);
	write_strln (":");

	/* Save allocation pointer on stack */
	write_strln ("  pushl __memp");
	write_strln ("  pushi");
	/* ..and reserve dummy local variable with index 1 */
	store_var (loc_p, "?");

	/* Arguments */
	if (!parse_argslist ())
	{
		return 0;
	}

	if (!read_sym (')'))
	{
		return 0;
	}

	/* Allow for declarations */

	if (read_sym (';'))
	{
		out_p = save;
		return 1;
	}

	/* Body always begins with opening curly brace */
	if (!read_sym ('{'))
	{
		return 0;
	}
	unread_sym ();

	/* Function body */
	if (!parse_block ())
	{
		return 0;
	}

	/* Function end label */
	write_str ("__");
	write_str (name);
	write_strln ("_end:");

	/* Release allocated memory (if any) */
	write_strln ("  pushl __memp");
	write_strln ("  pushsf");
	write_strln ("  push 1");
	write_strln ("  sub");
	write_strln ("  pushi");
	write_strln ("  popi");

	/* Return statement */
	write_strln ("  ret");

	/* Erase lists of args and locals */
	clear_memory (arg_p, ARG_SZ);
	clear_memory (loc_p, LOC_SZ);

	return 1;
}

int parse_root()
{
	char id[ID_SZ];
	write_strln ("  pushl __memp");
	write_strln ("  pushl __the_end");
	write_strln ("  popi");
	write_strln ("  pushl main");
	write_strln ("  call");
	write_strln ("  halt");
	write_strln ("__retval:");
	write_strln (" .word 0");
	write_strln ("__memp:");
	write_strln (" .word 0");
	/* Read identifier as a basis for any
	 * statement within the program's root. */
	while (read_id (id))
	{
		/* A function declaration */
		if (read_sym ('('))
		{
			if (!parse_func (id))
			{
				break;
			}
		}
		/* Global variable declaration and initialization */
		else if (read_sym ('='))
		{
			if (!parse_gvar (id))
			{
				break;
			}
		}
		/* Global array declaration */
		else if (read_sym ('['))
		{
			if (!parse_garr (id))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	write_strln ("__the_end:");
	/* At this moment we always return 1
	 * with no error checks */
	return 1;
}

/* Entry point */

int main()
{
	char source[SRC_SZ];
	char result[OUT_SZ];
	char locals[LOC_SZ];
	char arguments[ARG_SZ];
	char globals[GBL_SZ];

	clear_memory (source, SRC_SZ);
	clear_memory (result, OUT_SZ);
	clear_memory (locals, LOC_SZ);
	clear_memory (globals, GBL_SZ);
	clear_memory (arguments, ARG_SZ);

	src_p = source;
	out_p = result;
	loc_p = locals;
	arg_p = arguments;
	gbl_p = globals;

	while (1)
	{
		/* Read by character */
		*src_p = getchar ();

		/* Stop at EOF or overflow: do not change line below */
		if (((*src_p + 1) == 0) || (*src_p == 255))
		{
			break;
		}
		src_p = src_p + 1;
		if ((src_p - source) == SRC_SZ)
		{
			puts (";; Overflow!!");
			break;
		}
	}
	*src_p = 0;
	src_p = source;

	parse_root ();

	puts (";; Generated with FemtoC");
	puts (result);
	if (!*src_p)
	{
		puts (";; The end: no errors encountered");
	}
	else
	{
		puts (";; Error(s) found!");
	}

	return 0; /* SUCCESS */
}
