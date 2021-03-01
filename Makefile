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
	@echo "make test                 - run some functional tests"
run: all
	cat $(SRC) | ./cc > $(SRC).s
	as $(SRC).s -o $(SRC).o
	ld $(SRC).o -o $(SRC).bin
	./$(SRC).bin
	rm $(SRC).s $(SRC).o $(SRC).bin
clean:
	rm -f cc
test:	all
	@cd tests; \
	mkdir -p bin; \
	./smoke.sh; \
	./smoke-fail.sh; \
	./compare-compiled.sh; \
	./compare-stage2.sh
