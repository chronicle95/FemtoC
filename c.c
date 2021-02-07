/******************************************************************************
* This file is part of the FemtoC project (https://github.com/chronicle95).   *
* Copyright (c) 2017-2020 Artem Bondarenko.                                   *
*                                                                             *
* This program is free software: you can redistribute it and/or modify        *
* it under the terms of the GNU General Public License as published by        *
* the Free Software Foundation, version 3.                                    *
*                                                                             *
* This program is distributed in the hope that it will be useful, but         *
* WITHOUT ANY WARRANTY; without even the implied warranty of                  *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU            *
* General Public License for more details.                                    *
*                                                                             *
* You should have received a copy of the GNU General Public License           *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
******************************************************************************/

#include <stdio.h>

/* Limits */
#define ID_SZ   32     /* maximum identifier length */
#define SRC_SZ  64000  /* up to ~4k lines of C source code */
#define OUT_SZ  256000 /* up to ~20k lines of assembly output */
#define LOC_SZ  800    /* up to 20 local variables */
#define GBL_SZ  6400   /* up to 160 global identifiers (f + v) */
#define ARG_SZ  200    /* up to 5 arguments per function */
#define CNST_SZ 6400   /* up to 160 defined constants */
#define LINE_SZ 80     /* assumed line size for assembly */

/* Supported types */
#define TYPE_NONE  0
#define TYPE_INT   1
#define TYPE_CHR   2
#define TYPE_PTR   96  /* mask */
#define TYPE_PTR0  32
#define TYPE_PTR1  64
#define TYPE_ARR   128

/* Sections */
#define SECTION_TEXT 0
#define SECTION_DATA 1

/* Procedure declarations */
int parse_label();
int parse_statement();
int parse_loop_for();
int parse_loop_while();
int parse_conditional();
int parse_expr(int *type);
int type_sizeof(int type);

/* Global variables: Code arrays */
char source[SRC_SZ];
char result[OUT_SZ];

/* Global variables: Namespace arrays
 * Each namespace is a list of records, represented as a byte array.
 * Each record has the following format: [Marker][Type][Name]
 * where Marker is space character 0x20, Type is an 8 byte field and
 * Name is variable length character string.
 * End of namespace is marked with zero 0x00 instead of space. */
char locals[LOC_SZ];     /* function locals list */
char arguments[ARG_SZ];  /* function arguments list */
char globals[GBL_SZ]     /* global variable list */;
char constants[CNST_SZ]; /* constant list */

/* Global variables: Pointers */
char *src_p = 0; /* source code read pointer */
char *out_p = 0; /* assembly output write pointer */

/* Global variables: Loops work area */
int lbl_cnt = 0;     /* Label counter. Used for temporary labels */
char *lbl_sta = 0;   /* Nearest loop start label */
char *lbl_end = 0;   /* Nearest loop end label */

/* Global variables: Misc */
int line_number = 0; /* Current source line number */
char *last_str = 0;  /* Last output string */
int section = 0;     /* Linkage section */

/******************************************************************************
* Utility functions                                                           *
******************************************************************************/

int copy_memory(char *dst, char *src, int size) {
	while (size) {
		*dst = *src;
		dst = dst + 1;
		src = src + 1;
		size = size - 1;
	}
	return 0;
}

int clear_memory(char *p, int size) {
	while (size > 0) {
		*p = (char) 0;
		p = p + 1;
		size = size - 1;
	}
	return 0;
}

int gen_label(char *dst) {
	int n = lbl_cnt;
	copy_memory (dst, "_L_", 3);
	dst = dst + 3;
	if (n == 0) {
		*dst = 'a';
		dst = dst + 1;
	} else {
		while (n > 0) {
			*dst = 'a' + (n % 26);
			dst = dst + 1;
			n = n / 26;
		}
	}
	*dst = (char) 0;
	lbl_cnt = lbl_cnt + 1;
	return 1;
}

int strtonum(char *s) {
	int n = 0;
	while (*s) {
		n = n * 10 + (*s - '0');
		s = s + 1;
	}
	return n;
}

int strcomp(char *a, char *b) {
	if ((a == 0) && (b == 0)) {
		return 1;
	}
	if ((a == 0) || (b == 0)) {
		return 0;
	}
	while (*a && *b) {
		if (*a != *b)  {
			return 0;
		}
		a = a + 1;
		b = b + 1;
	}
	if (*a || *b) {
		return 0;
	}
	return 1;
}

int write_chr(char c) {
	*out_p = c;
	out_p = out_p + 1;
	*out_p = (char) 0;
	return 1;
}

int write_str(char *s) {
	char *last = last_str;
	while (*s) {
		write_chr (*s);
		*last = *s;
		last = last + 1;
		s = s + 1;
	}
	*last = (char) 0;
	return 1;
}

int write_strln(char *s) {
	write_str (s);
	write_chr (10);
	return 1;
}

int write_num(int n) {
	char buf[10];
	int i = 0;

	if (n == 0) {
		write_str ("0");
		return 1;
	}

	while (n > 0) {
		*(buf + i) = '0' + (n % 10);
		n = n / 10;
		i = i + 1;
	}

	i = i - 1;
	while (i > 0) {
		write_chr (*(buf + i));
		i = i - 1;
	}
	write_chr (*buf);

	return 1;
}

int write_err(char *s) {
	write_str ("# [ERROR][L ");
	write_num (line_number);
	write_str ("]: ");
	write_strln (s);
	return 0;
}

int write_numln(int n) {
	write_num (n);
	write_chr (10);
	return 1;
}

/**
 * Appends a variable to a varlist
 * A varlist is a sequence of characters, which are themselves
 * a series of names, separated by a space character.
 * Each name is prefixed with a single byte - the type of a
 * variable.
 * End of list is marked with null-character.
 *
 * @param[in] ptr pointer to a varlist
 * @param[in] type is variable type
 * @param[in] s pointer to a null-terminated name under question
 * @returns 1
 */
int store_var(char *ptr, int type, char *s) {
	char *fp = ptr;
	/* look for the end */
	while (*fp) {
		fp = fp + 1;                      /* skip space marker */
		fp = fp + type_sizeof (TYPE_INT); /* skip type */
		while (*fp && (*fp != ' ')) {     /* skip name */
			fp = fp + 1;
		}
	}
	/* mark the variable */
	*fp = ' ';
	fp = fp + 1;
	/* put type in there */
	*((int*) fp) = type;
	fp = fp + type_sizeof (TYPE_INT);
	/* append a word */
	while (*s) {
		*fp = *s;
		s = s + 1;
		fp = fp + 1;
	}
	/* null-terminate */
	*fp = (char) 0;
	return 1;
}

/**
 * Finds a variable index and type in a varlist
 *
 * @param[in] ptr pointer to a varlist
 * @param[in] s pointer to a null-terminated name under question
 * @param[out] t variable type
 * @param[out] i variable name index
 * @returns 1 on success, 0 if not found
 */
int find_var(char *ptr, char *s, int *t, int *i) {
	char *sp = NULL;
	char *fp = ptr;
	int tt = 0;
	char no_match = 0;
	*i = 0;

	while (*fp) {
		/* skip occupancy marker */
		fp = fp + 1;
		/* read type */
		tt = *((int*) fp);
		fp = fp + type_sizeof (TYPE_INT);
		/* read and check id */
		sp = s;
		no_match = 0;
		while (*fp && (*fp != ' ')) {
			if (*fp == *sp) {
				sp = sp + 1;
			} else {
				no_match = 1;
			}
			fp = fp + 1;
		}
		/* match? */
		if ((*sp == 0) && !no_match) {
			*t = tt;
			return 1;
		}
		/* end of list processing */
		if (!*fp) {
			break;
		}
		/* roll over to next var */
		*i = *i + 1;
	}

	*i = -1;

	return 0;
}

/******************************************************************************
* Character test functions                                                    *
******************************************************************************/

int is_space(char c) {
	return ((c == ' ') || (c == 10)
			|| (c == 13)  || (c == 9));
}

int is_digit(char c) {
	return ((c >= '0') && (c <= '9'));
}

int is_id0(char c) {
	return ((c == '_')
			|| ((c >= 'A') && (c <= 'Z'))
			|| ((c >= 'a') && (c <= 'z')));
}

int is_id(char c) {
	return is_id0 (c)
		|| is_digit (c);
}

/******************************************************************************
* Read functions                                                              *
******************************************************************************/

int read_space() {
	while (1) {
		/* Ignore comments */
		if ((*src_p == '/') && (*(src_p+1) == '*')) {
			while (!((*src_p == '*') && (*(src_p+1) == '/'))) {
				if (*src_p == 10) {
					line_number = line_number + 1;
				}
				src_p = src_p + 1;
			}
			src_p = src_p + 2;
		}

		/* Ignore spaces */
		if (is_space (*src_p)) {
			if (*src_p == 10) {
				line_number = line_number + 1;
			}
			src_p = src_p + 1;
		} else {
			break;
		}
	}
	return 1;
}

int read_sym(char exp) {
	read_space ();
	if (*src_p == exp) {
		src_p = src_p + 1;
		return 1;
	}
	return 0;
}

int read_sym_s(char *exp_s) {
	int count = 0;

	read_space ();

	while (*exp_s && (*src_p == *exp_s)) {
		src_p = src_p + 1;
		exp_s = exp_s + 1;
		count = count + 1;
	}

	if (*exp_s || is_id (*src_p)) {
		src_p = src_p - count;
		return 0;
	}

	return 1;
}

int unread_sym() {
	src_p = src_p - 1;
	return 1;
}

int read_str_const() {
	while (*src_p && (*src_p != '"')) {
		write_str (" ");
		write_num (*src_p);
		write_str (",");
		src_p = src_p + 1;
	}
	src_p = src_p + 1;
	return 1;
}

int type_reference(int type) {
	if ((type & TYPE_PTR) == TYPE_PTR) {
		write_err ("depth of reference exceeded");
		return type;
	}
	return type + TYPE_PTR0;
}

int type_dereference(int type) {
	if ((type & TYPE_PTR) == 0) {
		write_err ("can't dereference a non-pointer");
		return type;
	}
	return type - TYPE_PTR0;
}

int read_type(int *type) {
	int ref_depth = 0;
	read_space ();
	if (read_sym ('*')) {
		return 0;
	}
	if (read_sym_s ("int")) {
		*type = TYPE_INT;
	} else if (read_sym_s ("char")) {
		*type = TYPE_CHR;
	} else {
		return 0;
	}
	while (ref_depth < 3) {
		if (read_sym ('*')) {
			*type = type_reference (*type);
			ref_depth = ref_depth + 1;
		} else {
			break;
		}
	}
	return 1;
}

int read_id(char *dst) {
	char *dp = dst;

	read_space ();
	if (!is_id0 (*src_p)) {
		return 0;
	}
	while (is_id (*src_p)) {
		*dp = *src_p;
		dp = dp + 1;
		src_p = src_p + 1;
	}
	*dp = (char) 0;

	return 1;
}

int read_number(char *dst) {
	read_space ();

	if (!is_digit (*src_p)) {
		return 0;
	}

	while (is_digit (*src_p)) {
		*dst = *src_p;
		dst = dst + 1;
		src_p = src_p + 1;
	}

	*dst = (char) 0;
	return 1;
}

/******************************************************************************
* Code generation functions                                                   *
******************************************************************************/

int _gen_cmd_pop_rax() {
	if (!strcomp (last_str, "  push %rax")) {
		write_strln ("  pop %rax");
	} else {
		out_p = out_p - 12;
	}
	return 1;
}

int gen_cmd_swap() {
	_gen_cmd_pop_rax ();
	write_strln ("  pop %rbx");
	write_strln ("  push %rax");
	write_strln ("  push %rbx");
	return 1;
}

int gen_cmd_pushns(char *value) {
	write_str ("  movq $");
	write_str (value);
	write_strln (", %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pushni(int value) {
	write_str ("  movq $");
	write_num (value);
	write_strln (", %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pushl(char *name) {
	write_str ("  leaq ");
	write_str (name);
	write_strln ("(%rip), %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pushi(int type) {
	_gen_cmd_pop_rax ();
	if (type == TYPE_CHR) {
		write_strln ("  movzbq (%rax), %rax");
	} else {
		write_strln ("  movq (%rax), %rax");
	}
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_popi(int type) {
	_gen_cmd_pop_rax ();
	write_strln ("  pop %rbx");
	if (type == TYPE_CHR) {
		write_strln ("  mov %al, (%rbx)");
	} else {
		write_strln ("  movq %rax, (%rbx)");
	}
	return 1;
}

int gen_cmd_inv() {
	_gen_cmd_pop_rax ();
	write_strln ("  notq %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_add() {
	_gen_cmd_pop_rax ();
	write_strln ("  pop %rbx");
	write_strln ("  add %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_sub() {
	write_strln ("  pop %rbx");
	write_strln ("  pop %rax");
	write_strln ("  sub %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_call(char *name) {
	/* Save old base to stack, set a new base */
	write_strln ("  push %rbp");
	write_strln ("  movq %rsp, %rbp");
	write_str ("  call ");
	write_strln (name);
	/* Restore old base from stack */
	write_strln ("  pop %rbp");
	return 1;
}

int gen_cmd_pushsf() {
	write_strln ("  push %rbp");
	return 1;
}

int gen_cmd_jump(char *name) {
	write_str ("  jmp ");
	write_strln (name);
	return 1;
}

int gen_cmd_jump_x(char *prefix, char *name, char *suffix) {
	write_str ("  jmp ");
	write_str (prefix);
	write_str (name);
	write_strln (suffix);
	return 1;
}

int gen_cmd_nzjump(char *name) {
	_gen_cmd_pop_rax ();
	write_strln ("  cmpq $0, %rax");
	write_str ("  jne ");
	write_strln (name);
	return 1;
}

int gen_cmd_push_static(char *name, int type) {
	if (type == TYPE_CHR) {
		write_str ("  movzbq ");
	} else if (type & TYPE_ARR) {
		write_str ("  leaq ");
	} else {
		write_str ("  movq ");
	}
	write_str (name);
	write_strln ("(%rip), %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_pop_static(char *name, int type) {
	_gen_cmd_pop_rax ();
	write_str ("  movq %rax, ");
	write_str (name);
	write_strln ("(%rip)");
	return 1;
}

int gen_cmd_label_x(char *prefix, char *name, char *suffix) {
	write_str (prefix);
	write_str (name);
	write_str (suffix);
	write_strln (":");
	return 1;
}

int gen_cmd_label(char *name) {
	return gen_cmd_label_x("", name, "");
}

int gen_cmd_not() {
	_gen_cmd_pop_rax ();
	write_strln ("  or %rax, %rax");
	write_strln ("  sete %al");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_and() {
	_gen_cmd_pop_rax ();
	write_strln ("  pop %rbx");
	write_strln ("  and %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_or() {
	_gen_cmd_pop_rax ();
	write_strln ("  pop %rbx");
	write_strln ("  or %rbx, %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_dup() {
	_gen_cmd_pop_rax ();
	write_strln ("  push %rax");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_drop() {
	write_strln ("  pop %rax");
	return 1;
}

int _gen_cmd_cmp(char *cond) {
	_gen_cmd_pop_rax ();
	write_strln ("  pop %rbx");
	write_strln ("  xor %rdx, %rdx");
	write_strln ("  cmp %rax, %rbx");
	write_str ("  set");
	write_str (cond);
	write_strln (" %dl");
	write_strln ("  push %rdx");
	return 1;
}

int gen_cmd_cmpeq() {
	return _gen_cmd_cmp ("e");
}

int gen_cmd_cmpne() {
	return _gen_cmd_cmp ("ne");
}

int gen_cmd_cmplt() {
	return _gen_cmd_cmp ("l");
}

int gen_cmd_cmple() {
	return _gen_cmd_cmp ("le");
}

int gen_cmd_cmpgt() {
	return _gen_cmd_cmp ("g");
}

int gen_cmd_cmpge() {
	return _gen_cmd_cmp ("ge");
}

int gen_cmd_mul() {
	_gen_cmd_pop_rax ();
	write_strln ("  pop %rbx");
	write_strln ("  mulq %rbx");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_div() {
	write_strln ("  pop %rbx");
	write_strln ("  pop %rax");
	write_strln ("  xor %rdx,%rdx");
	write_strln ("  divq %rbx");
	write_strln ("  push %rax");
	return 1;
}

int gen_cmd_mod() {
	write_strln ("  pop %rbx");
	write_strln ("  pop %rax");
	write_strln ("  xor %rdx,%rdx");
	write_strln ("  divq %rbx");
	write_strln ("  push %rdx");
	return 1;
}

int gen_section(int sect) {
	if (sect != section) {
		section = sect;
		if (section == SECTION_TEXT) {
			write_strln (" .text");
		} else if (section == SECTION_DATA) {
			write_strln (" .data");
		} else {
			return 0;
		}
	}
	return 1;
}

int gen_start() {
	/* Use `puts` here instead of all `gen_cmd_*` stuff */
	puts ("# Generated with FemtoC");
	puts ("# GNU Assembler [as, x86_64]");
	puts (" .data");
	puts ("__mema:");
	puts (" .space 65536");
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
	puts ("  movq %rax, %rdi           # return code = %rax");
	puts ("  movq $60, %rax            # call = EXIT");
	puts ("  syscall");
	return 1;
}

/******************************************************************************
* Parse and process functions                                                 *
******************************************************************************/

int parse_invoke(char *name, int *ret_type) {
	int argcnt = 0;
	int n = 0;
	int len = 0;
	char *argpos[ARG_SZ / (ID_SZ + 1) + 1];
	int type = TYPE_INT;
	*ret_type = TYPE_INT;

	/* use argpos to locate where the output goes */
	*(argpos + argcnt) = out_p;

	while (1) {
		if (parse_expr (&type)) {
			argcnt = argcnt + 1;
			*(argpos + argcnt) = out_p;
			continue;
		} else if (read_sym (',')) {
			continue;
		} else if (read_sym (')')) {
			break;
		}
		return 0;
	}

	/* Now that we have all our arguments prepared,
	 * reverse them so the addressing is right */
	if (argcnt) {
		if (argcnt == 2) {
			gen_cmd_swap ();
		} else {
			len = *(argpos + argcnt) - *(argpos);
			copy_memory (*(argpos + argcnt), *(argpos), len);
			n = 0;
			while (n < argcnt) {
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
	while (n < argcnt) {
		gen_cmd_drop ();
		n = n + 1;
	}

	/* Determine the return type */
	if (!find_var (globals, name, ret_type, &n)) {
		write_err ("undeclared function");
		return 0;
	}

	return 1;
}

int type_sizeof(int type) {
	if (type == TYPE_CHR) return 1;
	if (type == TYPE_INT) return 8;
	if (type & TYPE_PTR) return 8;
	if (type & TYPE_ARR) return 8;
	return 0;
}

int parse_sizeof() {
	int type = TYPE_NONE;
	if (!read_sym ('(')) {
		write_err ("sizeof ( expected");
		return 0;
	}
	if (!read_type (&type)) {
		write_err ("sizeof bad type");
		return 0;
	}
	if (!read_sym (')')) {
		write_err ("sizeof ) expected");
		return 0;
	}
	gen_cmd_pushni (type_sizeof (type));
	return 1;
}

int parse_operand(int *type) {
	char buf[ID_SZ];
	char lbl[ID_SZ];
	int idx = 0;
	char *tmp = 0;
	int cast_type = TYPE_NONE;

	/* Type casting */
	tmp = src_p;
	if (read_sym ('(')) {
		if (read_type (&cast_type)) {
			if (!read_sym (')')) {
				write_err ("type cast expecting )");
				return 0;
			}
		} else {
			/* no type met, ignore what we've read */
			src_p = tmp;
		}
	}

	if (read_sym ('!')) {
		if (parse_operand (type)) {
			gen_cmd_not ();
			*type = TYPE_INT;
			goto _parse_operand_good;
		}
	} else if (read_sym ('&')) {
		if (!read_id (buf)) {
			return 0;
		}
		if (find_var (locals, buf, type, &idx)) {
			gen_cmd_pushsf ();
			gen_cmd_pushni ((idx + 1) * type_sizeof (TYPE_INT));
			gen_cmd_sub ();
		} else if (find_var (arguments, buf, type, &idx)) {
			gen_cmd_pushsf ();
			gen_cmd_pushni (idx * type_sizeof (TYPE_INT));
			gen_cmd_add ();
		} else if (find_var (globals, buf, type, &idx)) {
			gen_cmd_pushl (buf);
		} else {
			write_err ("undeclared identifier");
			return 0;
		}
		*type = type_reference (*type);
		goto _parse_operand_good;
	} else if (read_sym ('~')) {
		if (parse_operand (type)) {
			gen_cmd_inv ();
			*type = TYPE_INT;
			goto _parse_operand_good;
		}
	} else if (read_sym ('*')) {
		if (parse_operand (type)) {
			*type = type_dereference (*type);
			gen_cmd_pushi (*type);
			goto _parse_operand_good;
		}
	} else if (read_sym ('(')) {
		if (parse_expr (type)) {
			return read_sym (')');
		}
	} else if (read_sym ('"')) {
		gen_label (buf);
		gen_label (lbl);
		gen_cmd_jump (buf);
		gen_cmd_label (lbl);
		write_str (" .byte");
		read_str_const ();
		write_strln (" 0");
		gen_cmd_label (buf);
		gen_cmd_pushl (lbl);
		*type = type_reference (TYPE_CHR);
		goto _parse_operand_good;
	} else if (read_sym (39)) {
		gen_cmd_pushni (*src_p);
		src_p = src_p + 1;
		if (!read_sym (39)) {
			return 0;
		}
		*type = TYPE_CHR;
		goto _parse_operand_good;
	} else if (read_number (buf)) {
		gen_cmd_pushns (buf);
		*type = TYPE_INT;
		goto _parse_operand_good;
	} else if (read_sym_s ("NULL")) {
		gen_cmd_pushni (0);
		*type = type_reference (TYPE_NONE);
		goto _parse_operand_good;
	} else if (read_sym_s ("sizeof")) {
		if (!parse_sizeof ()) {
			return 0;
		}
		*type = TYPE_INT;
		goto _parse_operand_good;
	} else if (read_id (buf)) {
		if (read_sym ('(')) {
			if (!parse_invoke (buf, type)) {
				return 0;
			}
			write_strln ("  push %rax");
		} else {
			if (find_var (constants, buf, type, &idx)) {
				gen_cmd_pushni (*type);
				*type = TYPE_INT;
			} else if (find_var (locals, buf, type, &idx)) {
				gen_cmd_pushsf ();
				gen_cmd_pushni ((idx + 1) * type_sizeof (TYPE_INT));
				gen_cmd_sub ();
				gen_cmd_pushi (*type);
			} else if (find_var (arguments, buf, type, &idx)) {
				gen_cmd_pushsf ();
				gen_cmd_pushni (idx * type_sizeof (TYPE_INT));
				gen_cmd_add ();
				gen_cmd_pushi (*type);
			} else if (find_var (globals, buf, type, &idx)) {
				gen_cmd_push_static (buf, *type);
			} else {
				return 0;
			}
		}
		goto _parse_operand_good;
	} else if (read_sym ('-')) {
		if (parse_operand (type)) {
			gen_cmd_inv ();
			gen_cmd_pushni (1);
			gen_cmd_add ();
			goto _parse_operand_good;
		}
	}
	return 0;

_parse_operand_good:
	if (cast_type) {
		*type = cast_type;
	}
	return 1;
}

int pointer_math(char left_type, char right_type) {
	if ((left_type & TYPE_PTR) && !(right_type & TYPE_PTR)) {
		gen_cmd_pushni (type_sizeof (type_dereference (left_type)));
		gen_cmd_mul ();
	} else if (!(left_type & TYPE_PTR) && (right_type & TYPE_PTR)) {
		gen_cmd_swap ();
		gen_cmd_pushni (type_sizeof (type_dereference (left_type)));
		gen_cmd_mul ();
		gen_cmd_swap ();
	}
	return 1;
}

int parse_expr(int *type) {
	int tmp_type = TYPE_NONE;

	/* At least one operand is a basis for an expression */
	if (!parse_operand (type)) {
		return 0;
	}

	/* Any closing brackets, commas and semicolons
	 * are considered the end of expression */
	while (!read_sym (',')
			&& !read_sym (';')
			&& !read_sym (')')
			&& !read_sym (']')) {

		if (read_sym ('+')) {
			parse_operand (&tmp_type);
			pointer_math (*type, tmp_type);
			gen_cmd_add ();
		} else if (read_sym ('-')) {
			parse_operand (&tmp_type);
			pointer_math (*type, tmp_type);
			gen_cmd_sub ();
		} else if (read_sym ('*')) {
			parse_operand (&tmp_type);
			gen_cmd_mul ();
		} else if (read_sym ('/')) {
			parse_operand (&tmp_type);
			gen_cmd_div ();
		} else if (read_sym ('%')) {
			parse_operand (&tmp_type);
			gen_cmd_mod ();
		} else if (read_sym ('=')) {
			if (read_sym ('=')) {
				parse_operand (&tmp_type);
				gen_cmd_cmpeq ();
			} else {
				return 0;
			}
		} else if (read_sym ('<')) {
			if (read_sym ('=')) {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_cmple ();
			} else {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_cmplt ();
			}
		} else if (read_sym ('>')) {
			if (read_sym ('=')) {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_cmpge ();
			} else {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_cmpgt ();
			}
		} else if (read_sym ('!')) {
			if (!read_sym ('=')) {
				return 0;
			}
			if (!parse_operand (&tmp_type)) {
				return 0;
			}
			gen_cmd_cmpne ();
		} else if (read_sym ('&')) {
			if (read_sym ('&')) {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_not ();
				gen_cmd_swap ();
				gen_cmd_not ();
				gen_cmd_or ();
				gen_cmd_not ();
			} else {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_and ();
			}
		} else if (read_sym ('|')) {
			if (read_sym ('|')) {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_not ();
				gen_cmd_swap ();
				gen_cmd_not ();
				gen_cmd_and ();
				gen_cmd_not ();
			} else {
				if (!parse_operand (&tmp_type)) {
					return 0;
				}
				gen_cmd_or ();
			}
		} else {
			return 0;
		}
	}
	/* Parent rules should take care of the terminator */
	unread_sym ();
	return 1;
}

int parse_keyword_block() {
	if (read_sym_s ("if")) {
		if (!parse_conditional ()) {
			return 0;
		}
	} else if (read_sym_s ("while")) {
		if (!parse_loop_while ()) {
			return 0;
		}
	} else if (read_sym_s ("for")) {
		if (!parse_loop_for ()) {
			return 0;
		}
	} else if (read_sym_s ("asm")) {
		if (!read_sym ('{')) {
			write_err ("`{` expected");
			return 0;
		}
		write_strln("# ASM {");
		while (!read_sym ('}')) {
			read_space ();
			write_str ("  ");
			while ((*src_p != 10) && (*src_p != '}')) {
				write_chr (*src_p);
				src_p = src_p + 1;
			}
			write_chr (10);
		}
		write_strln("# } ASM");
	} else {
		return 0;
	}
	return 1;
}

int parse_block() {
	if (!read_sym ('{')) {
		if (parse_keyword_block ()) {
			return 1;
		}
		if (!parse_statement ()) {
			return 0;
		}
		if (!read_sym (';')) {
			return 0;
		}
	} else {
		while (!read_sym ('}')) {
			if (parse_keyword_block ()) {
				continue;
			}
			if (parse_label ()) {
				continue;
			}
			if (!parse_statement ()) {
				return 0;
			}
			if (!read_sym (';')) {
				return 0;
			}
		}
	}
	return 1;
}

int parse_conditional() {
	char lbl[ID_SZ];
	int type = TYPE_INT;  /* don't care */

	gen_label (lbl);

	if (!read_sym ('(')) {
		return 0;
	}

	if (!parse_expr (&type)) {
		return 0;
	}

	if (!read_sym (')')) {
		return 0;
	}

	gen_cmd_dup ();
	gen_cmd_not ();
	gen_cmd_nzjump (lbl);

	if (!read_sym (';')) {
		if (!parse_block ()) {
			return 0;
		}
	}

	if (read_sym_s ("else")) {
		gen_cmd_label (lbl);

		gen_label (lbl);
		gen_cmd_nzjump (lbl);

		if (!read_sym (';')) {
			if (!parse_block ()) {
				return 0;
			}
		}

		gen_cmd_label (lbl);
	} else {
		gen_cmd_label (lbl);
		gen_cmd_drop ();
	}

	return 1;
}

int parse_loop_for() {
	char lbl1[ID_SZ];
	char lbl2[ID_SZ];
	char lbl3[ID_SZ];
	char lbl4[ID_SZ];
	char *tmp_sta = 0;
	char *tmp_end = 0;
	int type = TYPE_INT; /* default */

	gen_label (lbl1);
	gen_label (lbl2);
	gen_label (lbl3);
	gen_label (lbl4);

	/* Remember parent loop labels if any */
	tmp_sta = lbl_sta;
	tmp_end = lbl_end;
	lbl_sta = lbl1;
	lbl_end = lbl2;

	if (!read_sym ('(')) {
		return 0;
	}

	/* First statement */
	while (!read_sym (';')) {
		if (!parse_statement (&type)) {
			return 0;
		}
		if (read_sym (',')) {
			continue;
		}
	}

	gen_cmd_label (lbl1);

	/* Second statement: conditional */
	if (!parse_expr (&type)) {
		return 0;
	}
	if (!read_sym (';')) {
		return 0;
	}
	gen_cmd_not ();
	gen_cmd_nzjump (lbl4);

	gen_cmd_jump (lbl3);

	gen_cmd_label (lbl2);

	/* Third statement */
	while (!read_sym (')')) {
		if (!parse_statement (&type)) {
			return 0;
		}
		if (read_sym (',')) {
			continue;
		}
	}

	gen_cmd_jump (lbl1);

	gen_cmd_label (lbl3);

	if (!read_sym (';')) {
		if (!parse_block ()) {
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

int parse_loop_while() {
	char lbl1[ID_SZ];
	char lbl2[ID_SZ];
	char *tmp_sta = 0;
	char *tmp_end = 0;
	int type = TYPE_INT; /* default */

	gen_label (lbl1);
	gen_label (lbl2);

	/* Remember parent loop labels if any */
	tmp_sta = lbl_sta;
	tmp_end = lbl_end;
	lbl_sta = lbl1;
	lbl_end = lbl2;

	if (!read_sym ('(')) {
		return 0;
	}

	gen_cmd_label (lbl1);

	if (!parse_expr (&type)) {
		return 0;
	}

	if (!read_sym (')')) {
		return 0;
	}

	gen_cmd_not ();
	gen_cmd_nzjump (lbl2);

	if (!read_sym (';')) {
		if (!parse_block ()) {
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

int parse_gvar(int type, char *name) {
	char num[ID_SZ];
	int numi = 0;
	int tmp = 0;

	gen_section (SECTION_DATA);
	gen_cmd_label (name);
	if (type == TYPE_CHR) {
		write_str (" .byte ");
	} else {
		write_str (" .quad ");
	}

	read_space ();
	if (read_number (num)) {
		write_strln (num);
	} else if (read_id (num)) {
		if (!find_var (constants, num, &numi, &tmp)) {
			return 0;
		}
		write_numln (numi);
	} else {
		return 0;
	}

	if (!read_sym (';')) {
		return 0;
	}

	store_var (globals, type, name);
	return 1;
}

int parse_garr(int type, char *name) {
	char num[ID_SZ];
	int tmp = 0;
	int numi = 0;

	gen_section (SECTION_DATA);
	gen_cmd_label (name);
	write_str (" .space ");
	write_num (type_sizeof (type));
	write_str ("*");

	if (read_number (num)) {
		write_strln (num);
	} else if (read_id (num)) {
		if (!find_var (constants, num, &numi, &tmp)) {
			return 0;
		}
		write_numln (numi);
	} else {
		return 0;
	}

	if (!read_sym (']')) {
		return 0;
	}
	if (!read_sym (';')) {
		return 0;
	}

	store_var (globals, type | TYPE_ARR, name);
	return 1;
}

int parse_label() {
	char id[ID_SZ];
	char *temp = src_p;
	if (!read_id (id)) {
		return 0;
	}
	if (!read_sym (':')) {
		src_p = temp;
		return 0;
	}
	gen_cmd_label_x ("__", id, "");
	return 1;
}

int parse_statement() {
	int idx = 0;
	char id[ID_SZ];
	char num[ID_SZ];

	int dst_type = TYPE_NONE;
	int type = TYPE_NONE;

	/* Show line of statement as comment */
	char *tmp = src_p;
	write_str ("# St.: ");
	while ((*tmp != ';') && (*tmp != 10) && (*tmp != '{')) {
		write_chr (*tmp);
		tmp = tmp + 1;
	}
	write_chr (10);

	/* Assignment by pointer */
	if (read_sym ('*')) {
		if (!parse_operand (&dst_type)) {
			return 0;
		}
		if (!read_sym ('=')) {
			return 0;
		}
		if (!parse_expr (&type)) {
			return 0;
		}
		dst_type = type_dereference (dst_type);
		if (type != dst_type) {
			write_err ("incompatible type assignment");
			return 0;
		}
		gen_cmd_popi (dst_type);
		return 1;
	}

	/*
	 *  Keywords checked first
	 */
	else if (read_sym_s ("return")) {
		/* calculate return value */
		if (!parse_expr (&type)) {
			return 0;
		}
		/* save it */
		_gen_cmd_pop_rax ();
		/* jump to end of function */
		char *id_p = id;
		char *fn_p = locals + 1 + type_sizeof (TYPE_INT);
		while (*fn_p && (*fn_p != ' ')) {
			*id_p = *fn_p;
			id_p = id_p + 1;
			fn_p = fn_p + 1;
		}
		*id_p = (char) 0;
		gen_cmd_jump_x ("__", id, "_end");
		return 1;
	} else if (read_sym_s ("goto")) {
		if (!read_id (id)) {
			write_err ("label expected");
			return 0;
		}
		gen_cmd_jump_x ("__", id, "");
		return 1;
	} else if (read_sym_s ("break")) {
		if (lbl_end) {
			gen_cmd_jump (lbl_end);
		}
		return 1;
	} else if (read_sym_s ("continue")) {
		if (lbl_sta) {
			gen_cmd_jump (lbl_sta);
		}
		return 1;
	} else if (read_type (&dst_type)) {
		if (!read_id (id)) {
			write_err ("identifier expected");
			return 0;
		}
		if (read_sym ('=')) {
			/* Definition of a local variable */
			if (!parse_expr (&type)) {
				return 0;
			}
			if (find_var (locals, id, &dst_type, &idx)
					|| find_var (globals, id, &dst_type, &idx)
					|| find_var (arguments, id, &dst_type, &idx)) {
				write_err ("duplicate identifier");
				return 0;
			} else {
				/* In a case of new allocation we need to provide
				 * space on stack for it so that we a have a place
				 * to store the value */
				gen_cmd_dup ();

				/* Allocate and point */
				store_var (locals, dst_type, id);
				find_var (locals, id, &dst_type, &idx);
				gen_cmd_pushsf ();
				gen_cmd_pushni ((idx + 1) * type_sizeof (TYPE_INT));
				gen_cmd_sub ();
			}
			gen_cmd_swap ();
			gen_cmd_popi (dst_type);
		} else if (read_sym ('[')) {
			/* Local array definition.
			 * It is not placed on stack but instead dynamically
			 * allocated from pool, only the pointer stays on
			 * stack */

			/* Initialize array pointer */
			write_strln ("  push %rdi");

			/* Calculate array length and leave it on the stack */
			if (!parse_expr (&type)) {
				return 0;
			}
			if (!read_sym (']')) {
				return 0;
			}

			/* Derive array size from length and element type */
			if (type_sizeof (dst_type) > 1)
			{
				write_str ("  movq $");
				write_num (type_sizeof (dst_type));
				write_strln (", %rax");
				write_strln ("  push %rax");
				gen_cmd_mul ();
			}

			/* Local arrays are considered pointers */
			store_var (locals, type_reference (dst_type), id);

			/* Allocate memory for the array */
			_gen_cmd_pop_rax ();
			write_strln ("  add %rax, %rdi");
		} else {
			write_err ("definition = or [ expected");
			return 0;
		}
		return 1;
	}

	/* Allow for empty statement */
	else if (read_sym (';')) {
		unread_sym ();
		return 1;
	}

	/* Otherwise check for identifier */
	else if (!read_id (id)) {
		write_err ("identifier expected");
		return 0;
	}


	/* Function call */
	if (read_sym ('(')) {
		return parse_invoke (id, &type);
	}

	/* Simple variable assignment */
	else if (read_sym ('=')) {
		if (!parse_expr (&type)) {
			return 0;
		}
		if (find_var (locals, id, &dst_type, &idx)) {
			gen_cmd_pushsf ();
			gen_cmd_pushni ((idx + 1) * type_sizeof (TYPE_INT));
			gen_cmd_sub ();
		} else if (find_var (arguments, id, &dst_type, &idx)) {
			gen_cmd_pushsf ();
			gen_cmd_pushni (idx * type_sizeof (TYPE_INT));
			gen_cmd_add ();
		} else if (find_var (globals, id, &dst_type, &idx)) {
			gen_cmd_pushl (id);
		} else {
			write_err ("undefined identifier");
			return 0;
		}
		gen_cmd_swap ();
		gen_cmd_popi (dst_type);
	}

	else {
		write_err ("bad statement");
		return 0;
	}

	return 1;
}

int parse_argslist() {
	char id[ID_SZ];
	int type = 0;

	while (!read_sym (')')) {
		if (!read_type (&type)) {
			write_err ("args: type expected");
			return 0;
		}

		if (!read_id (id)) {
			write_err ("args: identifier expected");
			return 0;
		}

		store_var (arguments, type, id);

		if (read_sym (',')) {
			continue;
		}
	}

	return 1;
}

int parse_func(int type, char *name) {
	char *save = out_p;

	/* Put function name to locals and arguments lists
	 * so that the first entry index starts with 1 */
	store_var (locals, type, name);
	store_var (arguments, type, name);

	/* Put function name to globals list */
	store_var (globals, type, name);

	/* Put label */
	gen_section (SECTION_TEXT);
	write_str (name);
	write_strln (":");

	/* Save allocation pointer on stack */
	write_strln ("  push %rdi");
	/* ..and reserve dummy local variable with index 1 */
	store_var (locals, TYPE_INT, "?");

	/* Arguments */
	if (!parse_argslist ()) {
		return 0;
	}

	/* Allow for declarations */
	if (read_sym (';')) {
		/* Nothing to be done here */
		out_p = save;
		/* Erase arguments list */
		clear_memory (arguments, ARG_SZ);
		return 1;
	}

	/* Body always begins with opening curly brace */
	if (!read_sym ('{')) {
		return 0;
	}
	unread_sym ();

	/* Function body */
	if (!parse_block ()) {
		return 0;
	}

	/* Default return value */
	write_strln ("  xor %rax, %rax");

	/* Function end label */
	gen_cmd_label_x ("__", name, "_end");

	/* Return statement */
	write_strln ("  movq %rbp, %rsp");
	write_strln ("  leaq -16(%rsp), %rsp");
	write_strln ("  pop %rdi");
	write_strln ("  ret");

	/* Erase lists of args and locals */
	clear_memory (arguments, ARG_SZ);
	clear_memory (locals, LOC_SZ);

	return 1;
}

int parse_preprocessor() {
	char id[ID_SZ];
	char num[ID_SZ];
	if (read_sym_s ("include")) {
		/* Not supported for now */
	} else if (read_sym_s("define")) {
		/* Allows defining constant numbers */
		if (!read_id (id)) {
			write_err ("define: identifier expected");
			return 0;
		}
		if (!read_number (num)) {
			write_err ("define: number expected");
			return 0;
		}
		store_var (constants, strtonum (num), id);
	} else {
		write_err ("unsupported preprocessor. use: include,define");
		return 0;
	}
	/* Find new line */
	while (*src_p && (*src_p != 10)) {
		src_p = src_p + 1;
	}
	return 1;
}

int parse_root() {
	char id[ID_SZ];
	int type = 0;

	while (*src_p) {
		/* Preprocessor mockup (allows the parser to ignore pp) */
		if (read_sym ('#')) {
			if (!parse_preprocessor ()) {
				break;
			}
			continue;
		}
		if (!*src_p) {
			break;
		}
		/* Everything else must start with a type and id */
		if (!read_type (&type)) {
			write_err ("type expected");
			return 0;
		}
		if (!read_id (id)) {
			write_err ("identifier expected");
			return 0;
		}
		/* Function declaration */
		if (read_sym ('(')) {
			if (!parse_func (type, id)) {
				break;
			}
		}
		/* Global variable declaration and initialization */
		else if (read_sym ('=')) {
			if (!parse_gvar (type, id)) {
				break;
			}
		}
		/* Global array declaration */
		else if (read_sym ('[')) {
			if (!parse_garr (type, id)) {
				break;
			}
		} else {
			write_err ("function or variable definition expected");
			break;
		}
	}
	/* At this moment we always return 1
	 * with no error checks */
	return 1;
}

/******************************************************************************
* Entry point                                                                 *
******************************************************************************/

int main() {
	char last_written_str[LINE_SZ];

	int  temp = 0;

	clear_memory (source, SRC_SZ);
	clear_memory (result, OUT_SZ);
	clear_memory (locals, LOC_SZ);
	clear_memory (globals, GBL_SZ);
	clear_memory (arguments, ARG_SZ);
	clear_memory (constants, CNST_SZ);

	src_p = source;
	out_p = result;
	last_str = last_written_str;

	while (1) {
		/* Read by character */
		*src_p = getchar ();

		/* Stop at EOF or overflow: do not change line below */
		if (((*src_p + 1) == 0) || (*src_p == 255)) {
			break;
		}
		src_p = src_p + 1;
		if ((src_p - source) == SRC_SZ) {
			puts ("# Overflow!!");
			break;
		}
	}
	*src_p = (char) 0;
	src_p = source;

	/* Here we go */
	parse_root ();

	if (find_var (globals, "main", &temp, &temp)) {
		/* only generate prologue when main function defined */
		gen_start ();
	}

	puts (result);
	if (!*src_p) {
		puts ("# The end: no errors encountered");
	} else {
		puts ("# Error(s) found!");
	}

	return 0; /* SUCCESS */
}
