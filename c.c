#include <stdio.h>

int is_space(char c)
{
	return (c == ' ' || c == '\t'
	      c == '\n' || c == '\r');
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
	if (!is_id0 (*p))
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
	if (!is_digit (*p))
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
