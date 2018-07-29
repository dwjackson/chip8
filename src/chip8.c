#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8.h"
#include "instructions.h"
#include "SDL.h"

#define USAGE_FMT "Usage: %s [FILE_NAME]\n"
#define LOAD_BUFSIZE 512
#define DISPLAY_WPIXELS 64
#define DISPLAY_HPIXELS 32

void chip8_init(struct chip8 *chip);
int chip8_load(struct chip8 *chip, char *file_name);
void chip8_exec(struct chip8 *chip, SDL_Renderer *renderer);
int decode(struct chip8 *chip, unsigned short ins, SDL_Renderer *renderer);

int main(int argc, char *argv[])
{
	struct chip8 chip;
	char *file_name;
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_bool done = SDL_FALSE;
	SDL_Event event;
	int disph, dispw;
	dispw = DISPLAY_WPIXELS * CHIP8_PIXEL_WIDTH;
	disph = DISPLAY_HPIXELS * CHIP8_PIXEL_HEIGHT;

	if (argc < 2) {
		printf(USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}

	chip8_init(&chip);
	file_name = argv[1];
	chip8_load(&chip, file_name);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		perror("SDL_Init");
		exit(EXIT_FAILURE);
	}
	if (SDL_CreateWindowAndRenderer(dispw, disph, 0, &window, &renderer) != 0) {
		SDL_Quit();
		perror("SDL_CreateWindowAndRenderer");
		exit(EXIT_FAILURE);
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	chip8_exec(&chip, renderer);
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				done = SDL_TRUE;
			}
		}
	}

	SDL_Quit();

	return 0;
}

void chip8_init(struct chip8 *chip)
{
	int i, j;

	memset(chip->reg_v, 0, CHIP8_REGCOUNT);
	chip->reg_i = 0;
	memset(chip->ram, 0, CHIP8_RAMBYTES);
	chip->pc = 0;
	chip->sp = 0;
	memset(chip->stack, 0, CHIP8_STACKSIZE * sizeof(unsigned short));
	for (i = 0 ; i < CHIP8_DISPLAYH; i++) {
		for (j = 0; j < CHIP8_DISPLAYW; j++) {
			chip->display[i][j] = 0x0;
		}
	}
}

int chip8_load(struct chip8 *chip, char *file_name)
{
	byte buf[LOAD_BUFSIZE];
	FILE *fp = fopen(file_name, "rb");
	size_t next = CHIP8_PROGSTART;
	size_t count;
	if (!fp) {
		return -1;
	}
	while ((count = fread(&buf, 1, LOAD_BUFSIZE, fp)) > 0) {
		if (next + count > CHIP8_RAMBYTES) {
			fprintf(stderr, "Program is too long\n");
			abort();
		}
		memmove(chip->ram + next, buf, count);
	}	
	fclose(fp);
	return 0;
}

void chip8_exec(struct chip8 *chip, SDL_Renderer *renderer)
{
	unsigned short ins;

	chip->pc = CHIP8_PROGSTART;
	while (chip->pc < CHIP8_RAMBYTES) {
		ins = (*(unsigned short *)&((chip->ram[chip->pc])));
		ins = TO_BIG_ENDIAN(ins);
		chip->pc += 2;
		if (decode(chip, ins, renderer) != 0) {
			break;
		}
	}
}

static void store_bcd(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte val = chip->reg_v[x];
	byte i = chip->reg_i;
	byte hundreds = val / 100;
	byte tens = (val - hundreds * 100) / 10;
	byte ones = val - hundreds * 100 - tens * 10;

	if (i + 2 >= CHIP8_RAMBYTES) {
		fprintf(stderr, "RAM overflow\n");
		abort();
	}

	chip->ram[i] = hundreds;
	chip->ram[i + 1] = tens;
	chip->ram[i + 2] = ones;
}

int decode(struct chip8 *chip, unsigned short ins, SDL_Renderer *renderer)
{
	byte nibble_h;
	byte x;
	byte y;
	byte n;
	int i;

	nibble_h = (ins & 0xF000) >> 12;
	if (nibble_h == 0x0) {
		y = ins & 0x00FF;
		if (y == 0x00) {
			/* NOP */
		} else if (y == 0xEE) {
			chip8_ret(chip);
		} else if (y == 0xFD) {
			/* EXIT */
			return 1;
		} else {
			fprintf(stderr, "Unrecognized instruction: 0x%04X\n", ins);
		}
	} else if (nibble_h == 0x1) {
		chip8_jump(chip, ins);
	} else if (nibble_h == 0x2) {
		chip8_call(chip, ins);
	} else if (nibble_h == 0x3) {
		chip8_sne(chip, ins);
	} else if (nibble_h == 0x6) {
		chip8_load_immediate(chip, ins);
	} else if (nibble_h == 0x7) {
		/* ADD Vx, byte */
		x = (ins & 0x0F00) >> 8;
		y = ins & 0x00FF;
		chip->reg_v[x] += y;
	} else if (nibble_h == 0x8) {
		x = (ins & 0x0F00) >> 8;
		y = (ins & 0x00F0) >> 4;
		n = ins & 0x000F;
		if (n == 0x4) {
			/* ADD Vx, Vy */
			chip->reg_v[x] = chip->reg_v[x] + chip->reg_v[y];
		} else {
			fprintf(stderr, "Not Implemented: 0x%04X\n", ins);
			abort();
		}
	} else if (nibble_h == 0xA) {
		chip8_load_i(chip, ins);
	} else if (nibble_h == 0xD) {
		/* DRW Vx, Vy, byte */
		chip8_draw(chip, ins, renderer);
	} else if (nibble_h == 0xF) {
		x = (ins & 0x0F00) >> 8;
		y = ins & 0x00FF;
		if (y == 0x07) {
			chip->reg_v[x] = chip->dt;
		} else if (y == 0x15) {
			chip->dt = chip->reg_v[x];
		} else if (y == 0x29) {
			/* LD F, Vx */
			/* TODO */
		} else if (y == 0x33) {
			/* LD B, Vx */
			store_bcd(chip, ins);
		} else if (y == 0x65) {
			/* LD Vx, [I] */
			for (i = 0; i < x; i++) {
				chip->reg_v[i] = chip->ram[chip->reg_i + i];
			}
		} else {
			fprintf(stderr, "Unrecognized instruction: 0x%04X\n", ins);
		}
	} else {
		fprintf(stderr, "Unrecognized instruction: 0x%04X\n", ins);
		abort();
	}

	return 0;
}

static void print_sprite(byte *sprite, int n)
{
	int i, j;
	int bit;
	for (i = 0; i < n; i++) {
		for (j = 0; j < 8; j++) {
			bit = BIT(sprite[i], j);
			if (bit == 1) {
				printf("*");
			} else {
				printf(" ");
			}
		}
		printf("\n");
	}
}

