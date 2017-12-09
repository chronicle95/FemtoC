#include <stdio.h>

/* Assembly language brief
 *
 * pushsf    - put stack frame absolute address onto head
 *             this address points to return address on stack 
 * push <n>  - put constant on stack
 * pushl <l> - put label address on stack
 * pushi     - pop stack head as address
 *             and push value at this address
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
 */


/* Global variables */
char ID_SZ = 20;

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
int lbl_cnt = 0; /* Label counter. Used for temporary labels */

/* Utility functions */

int gen_label(char *dst)
{
	int n;
	n = lbl_cnt;
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

int write_chr (char c)
{
	*out_p = c;
	out_p = out_p + 1;
	*out_p = 0;
	return 1;
}

int write_str (char *s)
{
	while (*s)
	{
		write_chr (*s);
		s = s + 1;
	}
	return 1;
}

int write_strln (char *s)
{
	write_str (s);
	write_chr (10);
	return 1;
}

int write_num (int n)
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

int write_numln (int n)
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
			|| (c == 13)  || (c == 8));
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
	int count;

	read_space ();

	count = 0;
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

int read_cstr(char* dst)
{
	if (!read_sym ('\"'))
	{
		return 0;
	}
	src_p = src_p + 1;
	while (*src_p != '\"')
	{
		if (*src_p == '\\')
		{
			src_p = src_p + 1;
		}
		*dst = *src_p;
		dst = dst + 1;
		src_p = src_p + 1;
	}
	*dst = 0;
	return 1;
}

int read_csym(char* dst)
{
	if (!read_sym ('\''))
	{
		return 0;
	}
	src_p = src_p + 1;
	while (*src_p != '\'')
	{
		if (*src_p == '\\')
		{
			src_p = src_p + 1;
		}
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
			return parse_invoke (buf);
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
			return 1;
		}
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
	char buf[ID_SZ];
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

	while (!read_sym ('}'))
	{
		if (!parse_statement ())
		{
			return 0;
		}
	}

	if (read_sym_s ("else"))
	{
		write_str (lbl);
		write_strln (":");

		gen_label (lbl);
		write_strln ("    not");
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

	gen_label (lbl1);
	gen_label (lbl2);

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

	return 1;
}

int parse_gvar(char* name)
{
	return 0;
}

int parse_garr(char* name)
{
	return 0;
}

int parse_statement ()
{
	char id[ID_SZ];

	while (!read_sym (';'))
	{
		/* Label `re_read` is used when internal construct
		 * ends with a symbol different from `;` */
re_read:
		if (!read_id (id))
		{
			return 0;
		}

		/* Keywords checked first */
		if (strcomp (id, "return"))
		{
			if (!parse_expr ())
			{
				return 0;
			}
			write_strln ("    ret");
			continue;
		}
		else if (strcomp (id, "if"))
		{
			if (!parse_conditional ())
			{
				return 0;
			}
			goto re_read;
		}
		else if (strcomp (id, "while"))
		{
			if (!parse_loop_while ())
			{
				return 0;
			}
			goto re_read;
		}
		else if (strcomp (id, "goto"))
		{
			if (!read_id (id))
			{
				return 0;
			}
			write_str ("    pushl __");
			write_strln (id);
			write_strln ("    jump");
			continue;
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
			/* TODO Assignment */
			write_str ("    ; Store to `");
			write_str (id);
			write_strln ("` variable");
		}

		/* Label */
		else if (read_sym (':'))
		{
			write_str ("__");
			write_str (id);
			write_strln (":");
		}

		else
		{
			return 0;
		}
	}

	return 1;
}

int parse_argslist ()
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
			return 0;
		}

		store_var (loc_p, id);

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
	/* At this moment we always return 1
	 * with no error checks */
	return 1;
}

/* Entry point */

int main()
{
	/* TODO: Dummy. Change in future */
	char source[1024];
	char result[1024];
	char locals[1024];
	char globals[1024];
	src_p = source;  *src_p = 0;
	out_p = result;  *out_p = 0;
	loc_p = locals;  *loc_p = 0;
	gbl_p = globals; *gbl_p = 0;
	printf ("Enter code:\n");
	fgets (source, sizeof (source), stdin);
	parse_root ();
	while (src_p != source)
	{
		putchar (' ');
		src_p = src_p - 1;
	}
	putchar ('^');
	printf ("\n----------------------\n");
	puts (result);
	return 0; /* SUCCESS */
}
