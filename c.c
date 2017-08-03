#include <stdio.h>

/* Global variables */
char* p = 0; /* source code pointer */

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
	while (is_space (*p))
	{
		p = p + 1;
	}
	return 1;
}

int read_sym(char exp)
{
	read_space ();
	if (*p == exp)
	{
		p = p + 1;
		return 1;
	}
	return 0;
}

int read_id(char* dst)
{
	read_space ();
	if (!is_id0 (*p))
	{
		return 0;
	}
	while (is_id (*p))
	{
		*dst = *p;
		dst = dst + 1;
		p = p + 1;
	}
	*dst = 0;
	return 1;
}

int read_number(char* dst)
{
	read_space ();
	if (!is_digit (*p))
	{
		return 0;
	}
	while (is_digit (*p))
	{
		*dst = *p;
		dst = dst + 1;
		p = p + 1;
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
	p = p + 1;
	while (*p != '\"')
	{
		if (*p == '\\')
		{
			p = p + 1;
		}
		*dst = *p;
		dst = dst + 1;
		p = p + 1;
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
	p = p + 1;
	while (*p != '\'')
	{
		if (*p == '\\')
		{
			p = p + 1;
		}
		*dst = *p;
		dst = dst + 1;
		p = p + 1;
	}
	*dst = 0;
	return 1;
}

/* Parse and process
 * functions */

int parse_expr()
{
	/* Any closing brackets, commas and semicolons
	 * are considered the end of expression */

}

int parse_func(char* name)
{
}

int parse_gvar(char* name)
{
}

int parse_garr(char* name)
{
}

int parse_root()
{
	char id[16];
	/* Read identifier as a basis for any
	 * statement within the program's root. */
	while (read_id (id))
	{
		/* Types are ignored */
		if (strcomp (id, "int") || strcomp (id, "char"))
		{
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
	/* At this moment we always return 1
	 * with no error checks */
	return 1;
}

/* Entry point */

int main()
{
	return 0; /* SUCCESS */
}
