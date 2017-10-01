CFLAGS := -std=c99 -g -D_XOPEN_SOURCE
VFLAGS := --leak-check=full --show-leak-kinds=all
ARCHIVOS = log.o player.o namegen.o confparser.o match.o protocol.h
PROGRAMA = main

all: clean $(PROGRAMA)

run: $(PROGRAMA)
	./$(PROGRAMA)

$(PROGRAMA): $(ARCHIVOS) $(PROGRAMA).o
	@mkdir -p fifos
	@rm -f fifos/*
	gcc $(CFLAGS) -o $(PROGRAMA) $^

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAMA) *.o
	rm -f fifos/*

valgrind: $(PROGRAMA)
	valgrind $(VFLAGS) ./$(PROGRAMA)

.PHONY: clean


