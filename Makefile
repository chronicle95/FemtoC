CC=gcc
AS=as
LD=ld

all: ./cc
./cc: c.c
	$(CC) c.c -o cc
help:
	@echo "make (all|)               - just build"
	@echo "make run SRC=<filename>   - build & run"
	@echo "make help                 - show this message"
	@echo "make clean                - remove redundant files"
run: all
	cat $(SRC) | ./cc > $(SRC).s
	rm $(SRC).s
clean:
	rm -f cc
