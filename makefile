CFLAGS := -g
VFLAGS := --leak-check=full --show-leak-kinds=all
ARCHIVOS = log.o player.o namegen.o confparser.o court.o protocol.o partners_table.o lock.o
PROGRAMA = main

all: clean $(PROGRAMA)

$(PROGRAMA): $(ARCHIVOS) $(PROGRAMA).o
	@mkdir -p fifos
	@rm -f fifos/*
	gcc -o $(PROGRAMA) $^

run: clean $(PROGRAMA)
	./$(PROGRAMA)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAMA) *.o
	touch ElLog.txt
	rm ElLog.txt
	rm -f fifos/*

valgrind: clean $(PROGRAMA)
	valgrind $(VFLAGS) ./$(PROGRAMA)

.PHONY: clean


