/* Testing runtime */
#include <stdio.h>
#include <stdlib.h>

#define BUF_SZ 4096

void show_info()
{
	puts ("Development runtime for FemtoC");
	puts ("Usage:");
	puts ("./rr <file>");
}

void assemble_file(const char *file_name, char *buf)
{
	/* TODO */
}

void execute_binary(char *buf)
{
	/* TODO */
}

void execute_file(const char *file_name)
{
	char *buffer;
	buffer = (char *) malloc (BUF_SZ);
	assemble_file (file_name, buffer);
	execute_binary (buffer);
	free (buffer);
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		show_info ();
		return 1;
	}
	execute_file (argv[1]);
	return 0;
}
