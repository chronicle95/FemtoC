all:
	gcc c.c -o cc
	gcc r.c -o rr
demo:	all
	cat examples/hw.fc | ./cc > hw.s
	./rr hw.s
clean:
	rm -f cc rr hw.s
