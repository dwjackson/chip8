#ifndef CHIP8_H
#define CHIP8_H

#define TO_BIG_ENDIAN(x) (((x) >> 8 & 0x00FF) | ((x) << 8 & 0xFF00))

#define CHIP8_REGCOUNT 16
#define CHIP8_RAMBYTES 4096
#define CHIP8_PROGSTART 0x200
#define CHIP8_SPRITEBYTES 15
#define CHIP8_STACKSIZE 16
#define CHIP8_DISPLAYH 32
#define CHIP8_DISPLAYW 64
#define CHIP8_FONTSTART 0x0
#define CHIP8_FONTWIDTH 5

typedef unsigned char byte;

struct chip8;

struct chip8_renderer {
	void *data;
	void (*render_display)(struct chip8 *chip);
};

struct chip8 {
	byte reg_v[CHIP8_REGCOUNT];
	byte vf; 
	unsigned int reg_i;
	byte ram[CHIP8_RAMBYTES];
	unsigned short pc;
	unsigned int dt;
	unsigned int st;
	unsigned short sp;
	unsigned short stack[CHIP8_STACKSIZE];
	byte display[CHIP8_DISPLAYH][CHIP8_DISPLAYW];
	byte (*waitkey)();
	struct chip8_renderer *renderer;
	int is_halted;
};

void chip8_init(struct chip8 *chip, byte (*waitkey)(),
	struct chip8_renderer *renderer);
int chip8_load(struct chip8 *chip, char *file_name);
void chip8_exec(struct chip8 *chip);
int chip8_decode(struct chip8 *chip, unsigned short ins);

#endif /* CHIP8_H */
