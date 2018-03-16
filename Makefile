all:
	gcc c.c -o cc
	gcc r.c -o rr
test:	all
	cat test.fc | ./cc > test.s
clean:
	rm -f cc rr test.s
