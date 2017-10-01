CFLAGS := -std=c99 -g -D_XOPEN_SOURCE
VFLAGS := --leak-check=full --show-leak-kinds=all
ARCHIVOS = log.o player.o namegen.o confparser.o match.o protocol.h
PROGRAMA = main

all: $(ARCHIVOS) $(PROGRAMA).o
	@mkdir -p fifos
	@rm -f fifos/*
	gcc $(CFLAGS) -o $(PROGRAMA) $^
	./$(PROGRAMA)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAMA) *.o
	rm -f fifos/*

valgrind: $(ARCHIVOS) $(PROGRAMA).o
	@mkdir -p fifos
	@rm -f fifos/*
	gcc $(CFLAGS) -o $(PROGRAMA) $^
	valgrind $(VFLAGS) ./$(PROGRAMA)

.PHONY: clean


