#include <stdio.h>

/* Assembly language brief
 * 
 * push <n>  - put constant on stack
 * pushl <l> - put label address on stack
 * pushi     - pop stack head as address
 *             and push value at this address
 * neg       - logically invert stack head
 * inv       - bitwise inversion of stack head
 * add       - pop two topmost stack values
 *             and push their sum
 * sub       - same as addition, but subtraction
 * mul       - same as addition, but multiplication
 * div       - same as addition, but division
 * ret       - return from subroutine
 * call      - call a subroutine (address is on top of stack,
 *             gets popped, return address is pushed)
 * jump      - unconditional go to label pointed at the stack head
 *             head is popped, as usual
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

/* Utility functions */

int strcomp(char* a, char* b)
{
	if (a == 0 && b == 0)
	{
		return 1;
	}
	if (a == 0 || b == 0)
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
		s++;
	}
	return 1;
}

int write_strln (char *s)
{
	write_str (s);
	write_chr (10);
}

int write_loc(char *s)
{
	char *fp = loc_p;
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

char *find_loc(char *s, int *i)
{
	char *sp = s;
	char *fp = loc_p;
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
	return (c == ' ' || c == 10
			|| c == 13  || c == 8);
}

int is_digit(char c)
{
	return (c >= '0' && c <= '9');
}

int is_id0(char c)
{
	return (c == '_'
			|| (c >= 'A' && c <= 'Z')
			|| (c >= 'a' && c <= 'z'));
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
	if (read_sym ('!'))
	{
		if (parse_operand())
		{
			write_strln ("    neg");
			return 1;
		}
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
		return parse_expr();
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
			write_str ("    pushl ");
			write_strln (buf);
			write_strln ("    pushi");
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
		else
		{
			return 0;
		}
	}
	/* Parent rules should take care of the terminator */
	unread_sym ();
	return 1;
}

int parse_conditional()
{
	return 0;
}

int parse_loop_while()
{
	return 0;
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
			goto re_read;
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

		write_loc (id);

		if (!read_sym (','))
		{
			break;
		}
	}

	return 1;
}

int parse_func(char* name)
{
	write_loc (name);

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
	src_p = source;  *src_p = 0;
	out_p = result;  *out_p = 0;
	loc_p = locals;  *loc_p = 0;
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
