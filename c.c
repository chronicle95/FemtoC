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

/* Procedure declarations */
int parse_statement();
int parse_loop_for();
int parse_loop_while();
int parse_conditional();

/* Global variables: Limits */
int ID_SZ = 32;     /* maximum identifier length */
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
int arch = 0;      /* Output type: 0 - default, 1 - GAS (Linux/x86) */

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
	write_str ("# [ERROR]: ");
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
		write_str (",");
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

int read_id_pp(char *dst)
{
	read_space ();

	if (*src_p == '#')
	{
		*dst = *src_p;
		dst = dst + 1;
		src_p = src_p + 1;
	}

	return read_id(dst);
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

/* Code generation functions */

int gen_cmd_swap()
{
	write_strln ("  pop %rax");
	write_strln ("  pop %rbx");
	write_strln ("  push %rax");
	write_strln ("  push %rbx");
	return 1;
}

int gen_cmd_pushns(char *value)
{
	write_str ("  movq $");
	write_str (value);
	write_strln (", %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pushni(int value)
{
	write_str ("  movq $");
	write_num (value);
	write_strln (", %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pushl(char *name)
{
	write_str ("  leaq ");
	write_str (name);
	write_strln ("(%rip), %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pushi()
{
	write_strln ("  pop %rax");
	write_strln ("  movq (%rax), %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_popi()
{
	write_strln ("  pop %rax");
	write_strln ("  pop %rbx");
	write_strln ("  movq %rax, (%rbx)");
	return 1;
}

int gen_cmd_inv()
{
	write_strln ("  pop %rax");
	write_strln ("  xor $ffffffffffffffff, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_add()
{
	write_strln ("  pop %rax");
	write_strln ("  pop %rbx");
	write_strln ("  add %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_sub()
{
	write_strln ("  pop %rbx");
	write_strln ("  pop %rax");
	write_strln ("  sub %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_call(char *name)
{
	/* Save old base to stack, set a new base */
	write_strln ("  push %rbp");
	write_strln ("  movq %rsp, %rbp");
	write_str ("  call ");
	write_strln (name);
	/* Restore old base from stack */
	write_strln ("  pop %rbp");
	write_strln ("  movq %rbp, %rsp");
	return 1;
}

int gen_cmd_pushsf()
{
	write_strln ("  push %rbp");
}

int gen_cmd_jump(char *name)
{
	write_str ("  jmp ");
	write_strln (name);
	return 1;
}

int gen_cmd_jump_x(char *prefix, char *name, char *suffix)
{
	write_str ("  jmp ");
	write_str (prefix);
	write_str (name);
	write_strln (suffix);
	return 1;
}

int gen_cmd_nzjump(char *name)
{
	write_strln ("  pop %rax");
	write_strln ("  cmp %rax, $0");
	write_str ("  jne ");
	write_strln (name);
	return 1;
}

int gen_cmd_push_static(char *name)
{
	write_str ("  movq ");
	write_str (name);
	write_strln ("(%rip), %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pop_static(char *name)
{
	write_strln ("  pop %rax");
	write_str ("  movq %rax, ");
	write_str (name);
	write_strln ("(%rip)");
	return 1;
}

int gen_cmd_label_x(char *prefix, char *name, char *suffix)
{
	write_str (prefix);
	write_str (name);
	write_str (suffix);
	write_strln (":");
	return 1;
}

int gen_cmd_label(char *name)
{
	return gen_cmd_label_x("", name, "");
}

int gen_cmd_not()
{
	write_strln ("  pop %rax");
	write_strln ("  or %rax, %rax");
	write_strln ("  sete %al");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_and()
{
	write_strln ("  pop %rax");
	write_strln ("  pop %rbx");
	write_strln ("  and %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_or()
{
	write_strln ("  pop %rax");
	write_strln ("  pop %rbx");
	write_strln ("  or %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_dup()
{
	write_strln ("  pop %rax");
	write_strln ("  push %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_drop()
{
	write_strln ("  pop %rax");
	return 1;
}

int gen_start()
{
	/* Use `puts` here instead of all `gen_cmd_*` stuff */
	puts ("# Generated with FemtoC");
	puts ("# GNU Assembler [as, x86_64]");
	puts (" .data");
	puts ("__mema:");
	puts (" .space 16777216");
	puts ("__mema_end:");

	/* Initialize data and stack pointers */
	puts (" .text");
	puts (" .global _start");
	puts ("_start:");
	puts ("  leaq __mema(%rip), %rdi");
	puts ("  leaq __mema_end-8(%rip), %rsp");
	puts ("  movq %rsp, %rbp");

	/* Call main() */
	puts ("  call main");

	/* Call exit(0) */
	puts ("  movq $60, %rax");
	puts ("  xor %rdi, %rdi");
	puts ("  syscall");
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
			gen_cmd_swap ();
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
	gen_cmd_call (name);

	/* Drop the arguments */
	n = 0;
	while (n < argcnt)
	{
		gen_cmd_drop ();
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
			gen_cmd_not ();
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
			gen_cmd_pushsf ();
			gen_cmd_pushni (idx);
			gen_cmd_sub ();
			/* account for ret addr: */
			gen_cmd_pushni (1);
			gen_cmd_sub ();
		}
		else if (find_var (arg_p, buf, &idx))
		{
			gen_cmd_pushsf ();
			gen_cmd_pushni (idx);
			gen_cmd_add ();
		}
		else if (find_var (gbl_p, buf, &idx))
		{
			gen_cmd_pushl (buf);
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
			gen_cmd_inv ();
			return 1;
		}
	}
	else if (read_sym ('*'))
	{
		if (parse_operand())
		{
			gen_cmd_pushi ();
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
		gen_cmd_jump (buf);
		gen_cmd_label (lbl);
		write_str (" .quad");
		read_str_const ();
		write_strln (" 0");
		gen_cmd_label (buf);
		gen_cmd_pushl (lbl);
		return 1;
	}
	else if (read_sym (39))
	{
		gen_cmd_pushni (*src_p);
		src_p = src_p + 1;
		if (!read_sym (39))
		{
			return 0;
		}
		return 1;
	}
	else if (read_number (buf))
	{
		gen_cmd_pushns (buf);
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
			write_strln ("  push %rax");
		}
		else
		{
			if (find_var (loc_p, buf, &idx))
			{
				gen_cmd_pushsf ();
				gen_cmd_pushni (idx);
				gen_cmd_sub ();
				gen_cmd_pushi ();
			}
			else if (find_var (arg_p, buf, &idx))
			{
				gen_cmd_pushsf ();
				gen_cmd_pushni (idx);
				gen_cmd_add ();
				gen_cmd_pushi ();
			}
			else if (find_var (gbl_p, buf, &idx))
			{
				gen_cmd_push_static (buf);
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
			gen_cmd_inv ();
			gen_cmd_pushni (1);
			gen_cmd_add ();
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
			gen_cmd_add ();
		}
		else if (read_sym ('-'))
		{
			parse_operand ();
			gen_cmd_sub ();
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
				gen_cmd_not ();
				gen_cmd_swap ();
				gen_cmd_not ();
				gen_cmd_or ();
				gen_cmd_not ();
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				gen_cmd_and ();
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
				gen_cmd_not ();
				gen_cmd_swap ();
				gen_cmd_not ();
				gen_cmd_and ();
				gen_cmd_not ();
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				gen_cmd_or ();
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
		write_strln("# ASM {");
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
		write_strln("# } ASM");
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

	gen_cmd_dup ();
	gen_cmd_not ();
	gen_cmd_nzjump (lbl);

	if (!read_sym (';'))
	{
		if (!parse_block ())
		{
			return 0;
		}
	}

	if (read_sym_s ("else"))
	{
		gen_cmd_label (lbl);

		gen_label (lbl);
		gen_cmd_nzjump (lbl);

		if (!read_sym (';'))
		{
			if (!parse_block ())
			{
				return 0;
			}
		}

		gen_cmd_label (lbl);
	}
	else
	{
		gen_cmd_label (lbl);
		gen_cmd_drop ();
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

	gen_cmd_label (lbl1);

	/* Second statement: conditional */
	if (!parse_expr ())
	{
		return 0;
	}
	if (!read_sym (';'))
	{
		return 0;
	}
	gen_cmd_not ();
	gen_cmd_nzjump (lbl4);

	gen_cmd_jump (lbl3);

	gen_cmd_label (lbl2);

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

	gen_cmd_jump (lbl1);

	gen_cmd_label (lbl3);

	if (!read_sym (';'))
	{
		if (!parse_block ())
		{
			return 0;
		}
	}

	gen_cmd_jump (lbl2);
	gen_cmd_label (lbl4);

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

	gen_cmd_label (lbl1);

	if (!parse_expr ())
	{
		return 0;
	}

	if (!read_sym (')'))
	{
		return 0;
	}

	gen_cmd_not ();
	gen_cmd_nzjump (lbl2);

	if (!read_sym (';'))
	{
		if (!parse_block ())
		{
			return 0;
		}
	}

	gen_cmd_jump (lbl1);
	gen_cmd_label (lbl2);

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
	gen_cmd_label (name);
	write_str (" .quad ");
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
	gen_cmd_label (name);
	write_str (" .space 8*");
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
		gen_cmd_popi ();
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
		write_strln ("  pop %rax");
		/* jump to end of function */
		idx = 0;
		while (*(loc_p + idx) != ' ')
		{
			*(id + idx) = *(loc_p + idx);
			idx = idx + 1;
		}
		*(id + idx) = 0;
		gen_cmd_jump_x ("__", id, "_end");
		return 1;
	}
	else if (read_sym_s ("goto"))
	{
		if (!read_id (id))
		{
			error_log ("label expected");
			return 0;
		}
		gen_cmd_jump_x ("__", id, "");
		return 1;
	}
	else if (read_sym_s ("break"))
	{
		if (lbl_end)
		{
			gen_cmd_jump (lbl_end);
		}
		return 1;
	}
	else if (read_sym_s ("continue"))
	{
		if (lbl_sta)
		{
			gen_cmd_jump (lbl_sta);
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
			gen_cmd_pushsf ();
			gen_cmd_pushni (idx);
			gen_cmd_sub ();
			/* account for ret addr: */
			gen_cmd_pushni (1);
			gen_cmd_sub ();
		}
		else if (find_var (arg_p, id, &idx))
		{
			gen_cmd_pushsf ();
			gen_cmd_pushni (idx);
			gen_cmd_add ();
		}
		else if (find_var (gbl_p, id, &idx))
		{
			gen_cmd_pushl (id);
		}
		else
		{
			/* In a case of new allocation we need to provide
			 * space on stack for it so that we a have a place
			 * to store the value */
			gen_cmd_dup ();

			/* Allocate and point */
			store_var (loc_p, id);
			find_var (loc_p, id, &idx);
			gen_cmd_pushsf ();
			gen_cmd_pushni (idx);
			gen_cmd_sub ();
		}
		gen_cmd_swap ();
		gen_cmd_popi ();
	}

	/* Label */
	else if (read_sym (':'))
	{
		gen_cmd_label_x ("__", id, "");
	}

	/* Local array definition.
	 * It is not placed on stack but instead dynamically
	 * allocated from pool, only the pointer stays on
	 * stack */
	else if (read_sym ('['))
	{
		/* Reserve memory cell for array pointer */
		gen_cmd_pushni (0);

		/* Calculate array size and leave it on the stack */
		if (!parse_expr ())
		{
			return 0;
		}
		if (!read_sym (']'))
		{
			return 0;
		}
		write_strln ("  push %rdi");
		gen_cmd_dup ();

		store_var (loc_p, id);
		find_var (loc_p, id, &idx);

		gen_cmd_pushsf ();
		gen_cmd_pushni (idx);
		gen_cmd_sub ();

		/* Initialize the array pointer */
		gen_cmd_swap ();
		gen_cmd_popi ();

		/* Allocate memory for the array */
		gen_cmd_add ();
		write_strln ("  pop %rdi");
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
	write_strln ("  push %rdi");
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
	gen_cmd_label_x ("__", name, "_end");

	/* Return statement */
	write_strln ("  pop %rdi");
	write_strln ("  ret");

	/* Erase lists of args and locals */
	clear_memory (arg_p, ARG_SZ);
	clear_memory (loc_p, LOC_SZ);

	return 1;
}

int parse_preprocessor(char *ppname)
{
	char atype[ID_SZ];
	if (strcomp (ppname, "include"))
	{
		/* Not supported for now */
	}
	else if (strcomp (ppname, "define"))
	{
		/* Not supported for now */
	}
	else
	{
		error_log ("unsupported preprocessor. use: include,define");
		return 0;
	}
	/* Find new line */
	while (*src_p && (*src_p != 10))
	{
		src_p = src_p + 1;
	}
	return 1;
}

int parse_root()
{
	char id[ID_SZ];
	/* Read identifier as a basis for any
	 * statement within the program's root. */
	while (read_id_pp (id))
	{
		/* Preprocessor */
		if (*id == '#')
		{
			if (!parse_preprocessor(id + 1))
			{
				break;
			}
			continue;
		}
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
	gen_cmd_label ("__the_end");
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
			puts ("# Overflow!!");
			break;
		}
	}
	*src_p = 0;
	src_p = source;

	parse_root ();

	gen_start ();
	puts (result);
	if (!*src_p)
	{
		puts ("# The end: no errors encountered");
	}
	else
	{
		puts ("# Error(s) found!");
	}

	return 0; /* SUCCESS */
}
