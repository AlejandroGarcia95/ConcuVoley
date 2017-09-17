CFLAGS := -std=c99
ARCHIVOS = log.o player.o namegen.o confparser.o
PROGRAMA = main

all: $(ARCHIVOS) $(PROGRAMA).o
	gcc -o $(PROGRAMA) $^
	./$(PROGRAMA)

%.o: %.c
	gcc -c $< -o $@ 

clean:
	rm -f $(PROGRAMA) *.o

valgrind: $(ARCHIVOS) $(PROGRAMA).o
	gcc -o $(PROGRAMA) $^
	valgrind --leak-check=yes ./$(PROGRAMA)

.PHONY: clean


