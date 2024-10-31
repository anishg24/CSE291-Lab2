MIN ?= 30
MAX ?= 40

all: main main.c
	@./main

main: main.c Makefile
	gcc -O0 main.c -DBOUND=$$(shuf -i $(MIN)-$(MAX) -n 1) -o main

main.S: main.c Makefile
	@gcc main.c -S -o main.S

.PHONY: clean

clean:
	rm -f main
