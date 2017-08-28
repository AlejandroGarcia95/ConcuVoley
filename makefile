CFLAGS := -std=c99
ARCHIVOS = log.o 
PROGRAMA = main

all: $(ARCHIVOS) $(PROGRAMA).o
	gcc -o $(PROGRAMA) $^
	./$(PROGRAMA)

%.o: %.c
	gcc -c $< -o $@ 

clean:
	rm -f $(PROGRAMA) *.o

.PHONY: clean


