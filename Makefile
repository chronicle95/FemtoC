all:
	gcc c.c -o cc
	gcc r.c -o rr
demo:	all
	cat hw.fc | ./cc > hw.s
	./rr hw.s
clean:
	rm -f cc rr hw.s
