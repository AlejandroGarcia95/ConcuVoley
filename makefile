GCC := -std=c99 -g
VFLAGS := --leak-check=full --show-leak-kinds=all
ARCHIVOS = log.o player.o namegen.o confparser.o match.o
PROGRAMA = main

all: clean $(PROGRAMA)

run: $(PROGRAMA)
	./$(PROGRAMA)

$(PROGRAMA): $(ARCHIVOS) $(PROGRAMA).o
	@mkdir -p fifos
	@rm -f fifos/*
	gcc -o $(PROGRAMA) $^

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAMA) *.o
	rm -f fifos/*
	rmdir fifos

valgrind: clean $(PROGRAMA)
	valgrind $(VFLAGS) ./$(PROGRAMA)

.PHONY: clean


