CFLAGS = -g -O0 -Wall -Wextra -ansi
CHIP8_SRC = main.c chip8.c instructions.c

all: dis8 chip8 asm8 txt2hex

chip8: $(CHIP8_SRC)
	cc $(CFLAGS) -I/usr/include/SDL2 -o chip8 $(CHIP8_SRC) -lSDL2

dis8: dis8.c
	cc $(CFLAGS) -o dis8 dis8.c

asm8: asm8.c
	cc $(CFLAGS) -o asm8 asm8.c

txt2hex: txt2hex.c
	cc $(CFLAGS) -o txt2hex txt2hex.c

clean:
	rm -f dis8
	rm -f chip8
	rm -f asm8
	rm -f txt2hex
	rm -f *.o

.PHONY: all clean
