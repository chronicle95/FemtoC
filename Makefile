CC=gcc

all: ./cc ./rr
./cc: c.c
	$(CC) c.c -o cc
./rr: r.c
	$(CC) r.c -o rr
help:
	@echo "make (all|)               - just build"
	@echo "make run SRC=<filename>   - build & run"
	@echo "make help                 - show this message"
	@echo "make clean                - remove redundant files"
run: all
	cat $(SRC) | ./cc > $(SRC).s
	./rr $(SRC).s
	rm $(SRC).s
clean:
	rm -f cc rr
