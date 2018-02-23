all:
	gcc c.c -o cc
test:	all
	cat test.fc | ./cc
