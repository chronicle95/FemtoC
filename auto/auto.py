#!/usr/bin/env python3
import sys

# Autogenerator
# Author: Artem Bondarenko
# Date: 02/09/2017
# Licence: GNU GPL v3

EMPTY = '$'
QUOTE = '\"'
RANGE = '#'

def check(gram):
    for key, value in gram.items():
        for opt in value:
            for param in opt:
                if param == EMPTY or param[0] == QUOTE:
                    continue
                if param[-1] == RANGE and len(param) == 3:
                    continue
                if not param in gram.keys():
                    print('[WARN] Rule `%s`: unknown parameter `%s`'%(key,param))

def write(gram):
    result = ''
    result += '/* Global variables */\n\n'
    result += 'char *code = 0;\n\n'
    result += '/* Forward declarations */\n\n'
    result += '\n'.join([('int parse_'+k+'();') for k in gram.keys()])
    result += '\n\n/* Reader methods */\n\n'
    result += 'int unread (int count)\n'
    result += '{\n'
    result += '    code = code - count;\n'
    result += '    return 1;\n'
    result += '}\n\n'
    result += 'int read (char *s)\n'
    result += '{\n'
    result += '    int count = 0;\n'
    result += '    while (*s)\n'
    result += '    {\n'
    result += '        if (*s != *code)\n'
    result += '        {\n'
    result += '            unread (count);\n'
    result += '            return 0;\n'
    result += '        }\n'
    result += '        s = s + 1;\n'
    result += '        code = code + 1;\n'
    result += '        count = count + 1;\n'
    result += '    }\n'
    result += '    return 1;\n'
    result += '}\n\n'
    result += 'int read_char (char low, char high)\n'
    result += '{\n'
    result += '    if ((*code >= low) && (*code <= high))\n'
    result += '    {\n'
    result += '        code = code + 1;\n'
    result += '        return 1;\n'
    result += '    }\n'
    result += '    return 0;\n'
    result += '}\n\n'
    for key, value in gram.items():
        ignore_failure = False
        result += 'int parse_' + key + '()\n'
        result += '{\n'
        result += '    char *ptr = code;\n'
        for opt in value:
            if opt[0] == EMPTY:
                ignore_failure = True
                continue
            result += '    '
            result += 'while ('
            if opt[0][0] == QUOTE:
                result += 'read (' + opt[0] + ')'
            elif opt[0][-1] == RANGE:
                result += '!read_char (\'%s\', \'%s\')'%(opt[0][0], opt[0][1])
            else:
                result += 'parse_' + opt[0] + ' ()'
            result += ')\n'
            result += '    {\n'
            for param in opt[1:]:
                result += '        '
                result += 'if ('
                if param[0] == QUOTE:
                    result += '!read (' + param + ')'
                elif param[-1] == RANGE:
                    result += '!read_char (\'%s\', \'%s\')'%(param[0], param[1])
                else:
                    result += '!parse_' + param + ' ()'
                result += ')\n'
                result += '        {\n'
                result += '            break;\n'
                result += '        }\n'
            result += '        return 1;\n'
            result += '    }\n'
        if ignore_failure:
            result += '    return 1;\n'
        else:
            result += '    code = ptr;\n'
            result += '    return 0;\n'
        result += '}\n\n' 
    return result

def parse_expr(text):
    expr = []
    token = ''
    escaped = False
    quoted = False
    text += ' '
    for i in range(len(text)):
        c = text[i]
        if escaped:
            token += c
            escaped = False
        elif quoted:
            if c == '\"':
                expr.append(token + c)
                token = ''
                quoted = False
            else:
                token += c
        elif c == '\\':
            escaped = True
        elif c == '\"':
            if token:
                expr.append(token)
            token = c
            quoted = True
        elif c.isspace():
            if token:
                expr.append(token)
                token = ''
        elif c == '|':
            if token:
                expr.append(token)
                token = ''
            if (i + 2) < len(text):
                return [expr] + parse_expr(text[i+1:])
        else:
            token += c
    return [expr]

def parse(text):
    gram = {}
    lines = list(filter(lambda x: x.strip(), text.splitlines()))
    for line in lines:
        parts = line.split('::==')
        if len(parts) != 2:
            print('[WARN] Bad line: ' + line)
            continue
        key = parts[0].strip()
        expr = parse_expr(parts[1].strip())
        if not expr:
            print('[WARN] Rule `%s` has empty expression'%(key,))
        gram[key] = expr
    return gram

def process(gram_s):
    g = parse(gram_s)
    check(g)
    return write(g)

def main(fn_gram, fn_outp):
    try:
        print('Loading file `%s`...' % (fn_gram,))
        gram_s = open(fn_gram).read()
        print('Building parser...')
        code_s = process(gram_s)
        print('Writing file `%s`...' % (fn_outp,))
        open(fn_outp, 'w').write(code_s)
    except IOError:
        print('File operation failed.')
    except Exception:
        print('Error happened')

def info():
    print('Automatic parser/translator generator v0.1')
    print('Usage: ' + sys.argv[0] + ' <grammar> <source>')
    print()
    print('C file will be generated in <source> specified')


if __name__=='__main__':
    if len(sys.argv) != 3:
        info()
    else:
        main(sys.argv[1], sys.argv[2])
