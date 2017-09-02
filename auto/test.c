/* Global variables */

char *code = 0;

/* Forward declarations */

int parse_id_symbols();
int parse_kwrd_return();
int parse_global_statement();
int parse_expression();
int parse_expr_compop();
int parse_id_sym();
int parse_global_var();
int parse_global_var_value();
int parse_data_type();
int parse_compop();
int parse_assignment();
int parse_statement();
int parse_expr_mulop();
int parse_pointer();
int parse_kwrd_while();
int parse_logop();
int parse_block();
int parse_local_var_value();
int parse_digit();
int parse_statements();
int parse_function_decl();
int parse_ws();
int parse_alpha();
int parse_identifier();
int parse_digits();
int parse_kwrd_if();
int parse_addop();
int parse_operand();
int parse_number();
int parse_mulop();
int parse_kwrd();
int parse_program();
int parse_kwrd_break();
int parse_function_call();
int parse_expr_addop();
int parse_local_var();
int parse_expr_unary();
int parse_global_statements();
int parse_id_first();
int parse_ws_l();
int parse_sign();

/* Reader methods */

int unread (int count)
{
    code = code - count;
    return 1;
}

int read (char *s)
{
    int count = 0;
    while (*s)
    {
        if (*s != *code)
        {
            unread (count);
            return 0;
        }
        s = s + 1;
        code = code + 1;
        count = count + 1;
    }
    return 1;
}

int read_char (char low, char high)
{
    if ((*code >= low) && (*code <= high))
    {
        code = code + 1;
        return 1;
    }
    return 0;
}

int parse_id_symbols()
{
    char *ptr = code;
    while (parse_id_sym ())
    {
        if (!parse_id_symbols ())
        {
            break;
        }
        return 1;
    }
    return 1;
}

int parse_kwrd_return()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("return"))
        {
            break;
        }
        if (!parse_expression ())
        {
            break;
        }
        if (!read (";"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_global_statement()
{
    char *ptr = code;
    while (parse_global_var ())
    {
        return 1;
    }
    while (parse_function_decl ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_expression()
{
    char *ptr = code;
    while (parse_expr_compop ())
    {
        if (!parse_ws_l ())
        {
            break;
        }
        if (!parse_logop ())
        {
            break;
        }
        if (!parse_expr_compop ())
        {
            break;
        }
        return 1;
    }
    while (parse_operand ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_expr_compop()
{
    char *ptr = code;
    while (parse_expr_mulop ())
    {
        if (!parse_compop ())
        {
            break;
        }
        if (!parse_expr_mulop ())
        {
            break;
        }
        return 1;
    }
    while (parse_operand ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_id_sym()
{
    char *ptr = code;
    while (parse_alpha ())
    {
        return 1;
    }
    while (parse_digit ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_global_var()
{
    char *ptr = code;
    while (parse_data_type ())
    {
        if (!parse_identifier ())
        {
            break;
        }
        if (!parse_global_var_value ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (";"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_global_var_value()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("="))
        {
            break;
        }
        if (!parse_number ())
        {
            break;
        }
        return 1;
    }
    while (parse_ws_l ())
    {
        if (!read ("["))
        {
            break;
        }
        if (!parse_number ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("]"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_data_type()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("char"))
        {
            break;
        }
        if (!parse_pointer ())
        {
            break;
        }
        return 1;
    }
    while (parse_ws_l ())
    {
        if (!read ("int"))
        {
            break;
        }
        if (!parse_pointer ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_compop()
{
    char *ptr = code;
    while (read (">="))
    {
        return 1;
    }
    while (read ("<="))
    {
        return 1;
    }
    while (read (">"))
    {
        return 1;
    }
    while (read ("<"))
    {
        return 1;
    }
    while (read ("=="))
    {
        return 1;
    }
    while (read ("!="))
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_assignment()
{
    char *ptr = code;
    while (parse_operand ())
    {
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("="))
        {
            break;
        }
        if (!parse_operand ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_statement()
{
    char *ptr = code;
    while (parse_local_var ())
    {
        return 1;
    }
    while (parse_function_call ())
    {
        return 1;
    }
    while (parse_kwrd ())
    {
        return 1;
    }
    while (parse_assignment ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_expr_mulop()
{
    char *ptr = code;
    while (parse_expr_addop ())
    {
        if (!parse_ws_l ())
        {
            break;
        }
        if (!parse_mulop ())
        {
            break;
        }
        if (!parse_expr_addop ())
        {
            break;
        }
        return 1;
    }
    while (parse_operand ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_pointer()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("*"))
        {
            break;
        }
        return 1;
    }
    return 1;
}

int parse_kwrd_while()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("while"))
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("("))
        {
            break;
        }
        if (!parse_expression ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (")"))
        {
            break;
        }
        if (!parse_block ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_logop()
{
    char *ptr = code;
    while (read ("&&"))
    {
        return 1;
    }
    while (read ("||"))
    {
        return 1;
    }
    while (read ("^^"))
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_block()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("{"))
        {
            break;
        }
        if (!parse_statements ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("}"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_local_var_value()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("="))
        {
            break;
        }
        if (!parse_number ())
        {
            break;
        }
        return 1;
    }
    while (parse_ws_l ())
    {
        if (!read ("["))
        {
            break;
        }
        if (!parse_number ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("]"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_digit()
{
    char *ptr = code;
    while (!read_char ('0', '9'))
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_statements()
{
    char *ptr = code;
    while (parse_statement ())
    {
        if (!parse_statements ())
        {
            break;
        }
        return 1;
    }
    return 1;
}

int parse_function_decl()
{
    char *ptr = code;
    while (parse_data_type ())
    {
        if (!parse_identifier ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("("))
        {
            break;
        }
        if (!parse_function_args ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (")"))
        {
            break;
        }
        if (!parse_block ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_ws()
{
    char *ptr = code;
    while (read (" "))
    {
        return 1;
    }
    while (read ("\r"))
    {
        return 1;
    }
    while (read ("\t"))
    {
        return 1;
    }
    while (read ("\n"))
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_alpha()
{
    char *ptr = code;
    while (!read_char ('A', 'Z'))
    {
        return 1;
    }
    while (!read_char ('a', 'z'))
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_identifier()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!parse_id_first ())
        {
            break;
        }
        if (!parse_id_symbols ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_digits()
{
    char *ptr = code;
    while (parse_digit ())
    {
        if (!parse_digits ())
        {
            break;
        }
        return 1;
    }
    return 1;
}

int parse_kwrd_if()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("if"))
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("("))
        {
            break;
        }
        if (!parse_expression ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (")"))
        {
            break;
        }
        if (!parse_block ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_addop()
{
    char *ptr = code;
    while (read ("+"))
    {
        return 1;
    }
    while (read ("-"))
    {
        return 1;
    }
    while (read ("&"))
    {
        return 1;
    }
    while (read ("|"))
    {
        return 1;
    }
    while (read ("^"))
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_operand()
{
    char *ptr = code;
    while (parse_expr_unary ())
    {
        return 1;
    }
    while (parse_number ())
    {
        return 1;
    }
    while (parse_ws_l ())
    {
        if (!read ("("))
        {
            break;
        }
        if (!parse_expression ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (")"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_number()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!parse_sign ())
        {
            break;
        }
        if (!parse_digit ())
        {
            break;
        }
        if (!parse_digits ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_mulop()
{
    char *ptr = code;
    while (read ("*"))
    {
        return 1;
    }
    while (read ("/"))
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_kwrd()
{
    char *ptr = code;
    while (parse_kwrd_if ())
    {
        return 1;
    }
    while (parse_kwrd_while ())
    {
        return 1;
    }
    while (parse_kwrd_break ())
    {
        return 1;
    }
    while (parse_kwrd_return ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_program()
{
    char *ptr = code;
    while (parse_global_statements ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_kwrd_break()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("break"))
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (";"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_function_call()
{
    char *ptr = code;
    while (parse_identifier ())
    {
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read ("("))
        {
            break;
        }
        if (!parse_call_args ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (")"))
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (";"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_expr_addop()
{
    char *ptr = code;
    while (parse_operand ())
    {
        if (!parse_ws_l ())
        {
            break;
        }
        if (!parse_addop ())
        {
            break;
        }
        if (!parse_operand ())
        {
            break;
        }
        return 1;
    }
    while (parse_operand ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_local_var()
{
    char *ptr = code;
    while (parse_data_type ())
    {
        if (!parse_identifier ())
        {
            break;
        }
        if (!parse_local_var_value ())
        {
            break;
        }
        if (!parse_ws_l ())
        {
            break;
        }
        if (!read (";"))
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_expr_unary()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("~"))
        {
            break;
        }
        if (!parse_operand ())
        {
            break;
        }
        return 1;
    }
    while (parse_ws_l ())
    {
        if (!read ("*"))
        {
            break;
        }
        if (!parse_operand ())
        {
            break;
        }
        return 1;
    }
    while (parse_ws_l ())
    {
        if (!read ("!"))
        {
            break;
        }
        if (!parse_operand ())
        {
            break;
        }
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_global_statements()
{
    char *ptr = code;
    while (parse_global_statement ())
    {
        if (!parse_global_statements ())
        {
            break;
        }
        return 1;
    }
    return 1;
}

int parse_id_first()
{
    char *ptr = code;
    while (read ("_"))
    {
        return 1;
    }
    while (parse_alpha ())
    {
        return 1;
    }
    code = ptr;
    return 0;
}

int parse_ws_l()
{
    char *ptr = code;
    while (parse_ws ())
    {
        if (!parse_ws_l ())
        {
            break;
        }
        return 1;
    }
    return 1;
}

int parse_sign()
{
    char *ptr = code;
    while (parse_ws_l ())
    {
        if (!read ("-"))
        {
            break;
        }
        return 1;
    }
    return 1;
}

