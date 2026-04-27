
CC      := gcc
CFLAGS  := -std=c89 -Wall -Wextra -pedantic
LDFLAGS := -lc -lncurses


.PHONY: release clean


release: bin


bin: main.o
	$(CC) -o $@ $< $(LDFLAGS)


main.o: main.c
	$(CC) $(CFLAGS) -o $@ -c $<


clean:
	$(RM) -r bin main.o
