#include <stdio.h>

/* Utility functions */

int strcmp(char *a, char *b)
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
			c == 13  || c == 8);
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

int read_space(char *p, int *i)
{
	while (is_space (*(p+*i)))
	{
		*i = *i + 1;
	}
	return 1;
}

int read_sym(char *p, int *i, char exp)
{
	read_space (p, i);
	if (*(p+*i) == exp)
	{
		*i = *i + 1;
		return 1;
	}
	return 0;
}

int read_id(char *p, int *i, char *dst)
{
	read_space (p, i);
	if (!is_id0 (*(p+*i)))
	{
		return 0;
	}
	while (is_id (*(p+*i)))
	{
		*dst = *(p+*i);
		dst = dst + 1;
		*i = *i + 1;
	}
	*dst = 0;
	return 1;
}

int read_number(char *p, int *i, char *dst)
{
	read_space (p, i);
	if (!is_digit (*(p+*i)))
	{
		return 0;
	}
	while (is_digit (*(p+*i)))
	{
		*dst = *(p+*i);
		dst = dst + 1;
		*i = *i + 1;
	}
	*dst = 0;
	return 1;
}

int read_cstr(char *p, int *i, char *dst)
{
	if (!read_sym (p, i, '\"'))
	{
		return 0;
	}
	*i = *i + 1;
	while (*(p+*i) != '\"')
	{
		if (*(p+*i) == '\\')
		{
			*i = *i + 1;
		}
		*dst = *(p+*i);
		dst = dst + 1;
		*i = *i + 1;
	}
	*dst = 0;
	return 1;
}

int read_csym(char *p, int *i, char *dst)
{
	if (!read_sym (p, i, '\''))
	{
		return 0;
	}
	*i = *i + 1;
	while (*(p+*i) != '\'')
	{
		if (*(p+*i) == '\\')
		{
			*i = *i + 1;
		}
		*dst = *(p+*i);
		dst = dst + 1;
		*i = *i + 1;
	}
	*dst = 0;
	return 1;
}

/* Parse and process
 * functions */

int parse_func(char *p, int *i, char *name)
{
}

int parse_gvar(char *p, int *i, char *name)
{
}

int parse_garr(char *p, int *i, char *name)
{
}

int parse_root(char *p, int *i)
{
	char id[16];
	/* Read identifier as a basis for any
	 * statement within the program's root. */
	while (read_id (p, i, id))
	{
		/* A function declaration */
		if (read_sym (p, i, '('))
		{
			if (!parse_func (p, i, id))
			{
				break;
			}
		}
		/* Global variable declaration and initialization */
		else if (read_sym (p, i, '='))
		{
			if (!parse_gvar (p, i, id))
			{
				break;
			}
		}
		/* Global array declaration */
		else if (read_sym (p, i, '['))
		{
			if (!parse_garr (p, i, id))
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
