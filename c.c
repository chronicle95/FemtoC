#include <stdio.h>

/* Assembly language brief
 * Implements stack machine on a linear
 * memory cell set.
 *
 * Memory model:
 *
 * +------+
 * | ArgN |
 *   ....
 * | Arg2 |
 * +------+
 * | Arg1 | <= function arguments
 * +------+
 * | Ret  | <= stack frame (holds return address)
 * +------+
 * | Loc1 | <= local variables
 * +------+
 * | Loc2 |
 *   ....
 * | LocN |
 * +------+
 *
 * Instruction set:
 *
 * pushsf    - put stack frame absolute address onto head
 *             this address points to return address on stack 
 * push <n>  - put constant on stack
 * pushl <l> - put label address on stack
 * pushi     - pop stack head as address
 *             and push value at this address
 * popi      - pop (stack-0) as value
 *             pop (stack-1) as address
 *             write value to cell at address
 * not       - logically invert stack head
 * inv       - bitwise inversion of stack head
 * add       - pop two topmost stack values
 *             and push their sum
 * sub       - same as addition, but subtraction
 * mul       - same as addition, but multiplication
 * div       - same as addition, but division
 * mod       - same as addtiion, but remainder of division
 * ret       - return from subroutine
 * call      - call a subroutine (address is on top of stack,
 *             gets popped, return address is pushed)
 * jump      - unconditional go to label pointed at the stack head
 *             head is popped, as usual
 * nzjump    - conditional jump if non-zero. stack head gets popped
 *             twice: first time - address, second time - value
 * cmpeq     - comparison operators. pop two top-most values and
 * cmpne       push the result on stack
 * cmplt
 * cmpgt
 * cmpge
 * cmple
 * and       - bitwise operation. two top-most stack values
 *             get replaced with the result
 * or        - bitwise operation. two top-most stack values
 *             get replaced with the result
 * swap      - two top-most stack values exchange their places
 * dup       - duplicate stack head
 * drop      - remove stack head
 *
 * Preprocessor directives:
 *  .byte ... - a set of constant bytes
 *  .zero <n> - sequence of N zeroes
 */


/* Global variables */
int ID_SZ = 24;
int SRC_SZ = 32000;
int OUT_SZ = 65000;
int LOC_SZ = 16000;
int GBL_SZ = 16000;

char* src_p = 0; /* source code pointer
		    points to current location */
char* out_p = 0; /* output pointer
		    points to current location */
char* loc_p = 0; /* function locals list.
		    points to the beginning.
		    record goes as follow:

		    f-arg f-arg f-var ... */
char* gbl_p = 0; /* global variable list.
		    points to the beginning.
		    record goes as follow:

		    var0 var1 ... */
int lbl_cnt = 0;   /* Label counter. Used for temporary labels */
char* lbl_sta = 0; /* Nearest loop start label */
char* lbl_end = 0; /* Nearest loop end label */

/* Utility functions */

int clear_memory(char *p, int size)
{
	while (size > 0)
	{
		*p = 0;
		p = p + 1;
		size = size - 1;
	}
}

int gen_label(char *dst)
{
	int n = lbl_cnt;
	*dst = '_';
	dst = dst + 1;
	*dst = 'L';
	dst = dst + 1;
	*dst = '_';
	dst = dst + 1;
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
	while (1)
	{
		if (parse_expr () || read_sym (','))
		{
			continue;
		}
		else if (read_sym (')'))
		{
			break;
		}
		return 0;
	}

	write_str ("    pushl ");
	write_strln (name);
	write_strln ("    call");

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
			write_strln ("    not");
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
			write_strln ("    pushsf");
			write_str ("    push ");
			write_numln (idx);
			write_strln ("    add");
		}
		else if (find_var (gbl_p, buf, &idx))
		{
			write_str ("    pushl ");
			write_strln (buf);
		}
		else
		{
			return 0;
		}
		return 1;
	}
	else if (read_sym ('~'))
	{
		if (parse_operand())
		{
			write_strln ("    inv");
			return 1;
		}
	}
	else if (read_sym ('*'))
	{
		if (parse_operand())
		{
			write_strln ("    pushi");
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
		write_str ("    pushl ");
		write_strln (buf);
		write_strln ("    jump");
		write_str (lbl);
		write_strln (":");
		write_str ("   .byte");
		read_str_const ();
		write_strln (" 0");
		write_str (buf);
		write_strln (":");
		write_str ("    pushl ");
		write_strln (lbl);
		return 1;
	}
	else if (read_sym (39))
	{
		write_str ("    push ");
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
		write_str ("    push ");
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
			write_strln ("    pushl __retval");
			write_strln ("    pushi");
		}
		else
		{
			if (find_var (loc_p, buf, &idx))
			{
				write_strln ("    pushsf");
				write_str ("    push ");
				write_numln (idx);
				write_strln ("    add");
				write_strln ("    pushi");
			}
			else if (find_var (gbl_p, buf, &idx))
			{
				write_str ("    pushl ");
				write_strln (buf);
				write_strln ("    pushi");
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
			write_strln ("    inv");
			write_strln ("    push 1");
			write_strln ("    add");
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
			write_strln ("    add");
		}
		else if (read_sym ('-'))
		{
			parse_operand ();
			write_strln ("    sub");
		}
		else if (read_sym ('*'))
		{
			parse_operand ();
			write_strln ("    mul");
		}
		else if (read_sym ('/'))
		{
			parse_operand ();
			write_strln ("    div");
		}
		else if (read_sym ('%'))
		{
			parse_operand ();
			write_strln ("    mod");
		}
		else if (read_sym ('='))
		{
			if (read_sym ('='))
			{
				parse_operand ();
				write_strln ("    cmpeq");
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
				write_strln ("    cmple");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("    cmplt");
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
				write_strln ("    cmpge");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("    cmpgt");
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
			write_strln ("    cmpne");
		}
		else if (read_sym ('&'))
		{
			if (read_sym ('&'))
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("    not");
				write_strln ("    swap");
				write_strln ("    not");
				write_strln ("    or");
				write_strln ("    not");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("    and");
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
				write_strln ("    not");
				write_strln ("    swap");
				write_strln ("    not");
				write_strln ("    and");
				write_strln ("    not");
			}
			else
			{
				if (!parse_operand ())
				{
					return 0;
				}
				write_strln ("    or");
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

int parse_statement();
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

	write_strln ("    dup");
	write_strln ("    not");
	write_str ("    pushl ");
	write_strln (lbl);
	write_strln ("    nzjump");

	if (!read_sym ('{'))
	{
		if (!parse_statement ())
		{
			return 0;
		}
	}
	else
	{
		while (!read_sym ('}'))
		{
			if (!parse_statement ())
			{
				return 0;
			}
		}
	}

	if (read_sym_s ("else"))
	{
		write_str (lbl);
		write_strln (":");

		gen_label (lbl);
		write_str ("    pushl ");
		write_strln (lbl);
		write_strln ("    nzjump");

		if (read_sym ('{'))
		{
			while (!read_sym ('}'))
			{
				if (!parse_statement ())
				{
					return 0;
				}
			}
		}
		else if (!parse_statement ())
		{
			return 0;
		}

		write_str (lbl);
		write_strln (":");
	}
	else
	{
		write_str (lbl);
		write_strln (":");
		write_strln ("    drop");
	}

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

	write_strln ("    not");
	write_str ("    pushl ");
	write_strln (lbl2);
	write_strln ("    nzjump");

	if (read_sym ('{'))
	{
		while (!read_sym ('}'))
		{
			if (!parse_statement ())
			{
				return 0;
			}
		}
	}
	else if (!parse_statement ())
	{
		return 0;
	}

	write_str ("    pushl ");
	write_strln (lbl1);
	write_strln ("    jump");
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
	write_str ("   .byte ");
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
	write_str ("   .zero ");
	write_strln (num);
	store_var (gbl_p, name);
	return 1;
}

int parse_statement()
{
	int idx = 0;
	char id[ID_SZ];
	char num[ID_SZ];

	/* Allow for empty statement */
	if (read_sym (';'))
	{
		return 1;
	}
	else if (read_sym ('*'))
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
		write_strln ("    popi");
		goto semicolon_end;
	}

	/*
	 *  Keywords checked first
	 */
	else if (read_sym_s ("return"))
	{
		if (!parse_expr ())
		{
			return 0;
		}
		write_strln ("    pushl __retval");
		write_strln ("    swap");
		write_strln ("    popi");
		write_strln ("    ret");
		goto semicolon_end;
	}
	else if (read_sym_s ("if"))
	{
		if (!parse_conditional ())
		{
			return 0;
		}
		/* brace or semicolon */
		return 1;
	}
	else if (read_sym_s ("while"))
	{
		if (!parse_loop_while ())
		{
			return 0;
		}
		/* brace or semicolon */
		return 1;
	}
	else if (read_sym_s ("goto"))
	{
		if (!read_id (id))
		{
			error_log ("label expected");
			return 0;
		}
		write_str ("    pushl __");
		write_strln (id);
		write_strln ("    jump");
		goto semicolon_end;
	}
	else if (read_sym_s ("break"))
	{
		if (lbl_end)
		{
			write_str ("    pushl ");
			write_strln (lbl_end);
			write_strln ("    jump");
		}
		goto semicolon_end;
	}
	else if (read_sym_s ("continue"))
	{
		if (lbl_sta)
		{
			write_str ("    pushl ");
			write_strln (lbl_sta);
			write_strln ("    jump");
		}
		goto semicolon_end;
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
			write_strln ("    pushsf");
			write_str ("    push ");
			write_numln (idx);
			write_strln ("    add");
		}
		else if (find_var (gbl_p, id, &idx))
		{
			write_str ("    pushl ");
			write_strln (id);
		}
		else
		{
			store_var (loc_p, id);
			find_var (loc_p, id, &idx);
			write_strln ("    pushsf");
			write_str ("    push ");
			write_numln (idx);
			write_strln ("    add");
		}
		write_strln ("    swap");
		write_strln ("    popi");
		goto semicolon_end;
	}

	/* Label */
	else if (read_sym (':'))
	{
		write_str ("__");
		write_str (id);
		write_strln (":");
		return 1;
	}

	/* Local array definition */
	else if (read_sym ('['))
	{
		if (!parse_expr ())
		{
			return 0;
		}
		if (!read_sym (']'))
		{
			return 0;
		}
		write_strln ("    pushl __memp");
		write_strln ("    pushi");
		write_strln ("    dup");

		store_var (loc_p, id);
		find_var (loc_p, id, &idx);
		write_strln ("    pushsf");
		write_str ("    push ");
		write_numln (idx);
		write_strln ("    add");

		write_strln ("    swap");
		write_strln ("    popi");
		write_strln ("    add");
		write_strln ("    pushl __memp");
		write_strln ("    swap");
		write_strln ("    popi");

		goto semicolon_end;
	}

	else
	{
		error_log ("bad statement");
		return 0;
	}

semicolon_end:
	if (!read_sym (';'))
	{
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

		store_var (loc_p, id);
		/* TODO: write number from stack
		 * to local variable (pop) */

		if (!read_sym (','))
		{
			break;
		}
	}

	return 1;
}

int parse_func(char* name)
{
	/* Put function name to locals list so that
	 * first argument index starts with 1 */
	store_var (loc_p, name);

	/* Put function name to globals list */
	store_var (gbl_p, name);

	/* Put label */
	write_str (name);
	write_strln (":");

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
		return 1;
	}

	/* Statements */

	if (!read_sym ('{'))
	{
		return 0;
	}

	while (!read_sym ('}'))
	{
		if (!parse_statement ())
		{
			return 0;
		}
	}

	/* Return statement */
	write_strln ("    ret");

	return 1;
}

int parse_root()
{
	char id[ID_SZ];
	write_strln ("    pushl __memp");
	write_strln ("    pushl __the_end");
	write_strln ("    popi");
	write_strln ("    pushl main");
	write_strln ("    call");
	write_strln ("    halt");
	write_strln ("getchar:");
	write_strln ("    pushl __retval");
	write_strln ("    input");
	write_strln ("    popi");
	write_strln ("    ret");
	write_strln ("putchar:");
	write_strln ("    pushsf");
	write_strln ("    push 1");
	write_strln ("    add");
	write_strln ("    pushi");
	write_strln ("    output");
	write_strln ("    ret");
	write_strln ("__retval:");
	write_strln ("   .byte 0");
	write_strln ("__memp:");
	write_strln ("   .byte 0");
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
	char globals[GBL_SZ];

	clear_memory (source, SRC_SZ);
	clear_memory (result, OUT_SZ);
	clear_memory (locals, LOC_SZ);
	clear_memory (globals, GBL_SZ);

	src_p = source;  *src_p = 0;
	out_p = result;  *out_p = 0;
	loc_p = locals;  *loc_p = 0;
	gbl_p = globals; *gbl_p = 0;

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
