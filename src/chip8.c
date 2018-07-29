#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8.h"
#include "instructions.h"
#include "SDL.h"
#include <time.h>

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
	int ret;

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
	ret = SDL_CreateWindowAndRenderer(dispw, disph, 0, &window, &renderer);
	if (ret != 0) {
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
	unsigned short addr;
	time_t now;
	byte fontchars[16][CHIP8_FONTWIDTH] = {
		{ 0xF0, 0x90, 0x90, 0x90, 0xF0 }, /* 0 */
		{ 0x20, 0x60, 0x20, 0x20, 0x70 }, /* 1 */
		{ 0xF0, 0x10, 0xF0, 0x80, 0xF0 }, /* 2 */
		{ 0xF0, 0x10, 0xF0, 0x10, 0xF0 }, /* 3 */
		{ 0x90, 0x90, 0xF0, 0x10, 0x10 }, /* 4 */
		{ 0xF0, 0x80, 0xF0, 0x10, 0xF0 }, /* 5 */
		{ 0xF0, 0x80, 0xF0, 0x90, 0xF0 }, /* 6 */
		{ 0xF0, 0x10, 0x20, 0x40, 0x40 }, /* 7 */
		{ 0xF0, 0x90, 0xF0, 0x90, 0xF0 }, /* 8 */
		{ 0xF0, 0x90, 0xF0, 0x10, 0xF0 }, /* 9 */
		{ 0xF0, 0x90, 0xF0, 0x90, 0x90 }, /* A */
		{ 0xE0, 0x90, 0xE0, 0x90, 0xE0 }, /* B */
		{ 0xF0, 0x80, 0x80, 0x80, 0xF0 }, /* C */
		{ 0xF0, 0x90, 0x90, 0x90, 0xE0 }, /* D */
		{ 0xF0, 0x80, 0xF0, 0x80, 0xF0 }, /* E */
		{ 0xF0, 0x80, 0xF0, 0x80, 0x80 }  /* F */ 
	};	

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
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 5; j++) {
			addr = CHIP8_FONTSTART + i * CHIP8_FONTWIDTH + j;
			chip->ram[addr] = fontchars[i][j];
		}
	}
	now = time(NULL);
	srand(now);
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

int decode(struct chip8 *chip, unsigned short ins, SDL_Renderer *renderer)
{
	byte nibble_h;
	byte y;
	byte n;

	nibble_h = (ins & 0xF000) >> 12;
	if (nibble_h == 0x0) {
		y = ins & 0x00FF;
		if (y == 0x00) {
			/* NOP */
		} else if (y == 0x0E) {
			chip8_cls(chip, renderer);
		} else if (y == 0xEE) {
			chip8_ret(chip);
		} else if (y == 0xFD) {
			/* EXIT */
			return 1;
		} else {
			fprintf(stderr, "Unrecognized instruction: 0x%04X\n",
				ins);
		}
	} else if (nibble_h == 0x1) {
		chip8_jump(chip, ins);
	} else if (nibble_h == 0x2) {
		chip8_call(chip, ins);
	} else if (nibble_h == 0x3) {
		chip8_se(chip, ins);
	} else if (nibble_h == 0x4) {
		chip8_sne(chip, ins);
	} else if (nibble_h == 0x6) {
		chip8_load_immediate(chip, ins);
	} else if (nibble_h == 0x7) {
		chip8_add_immediate(chip, ins);
	} else if (nibble_h == 0x8) {
		n = ins & 0x000F;
		if (n == 0x0) {
			chip8_ld(chip, ins);
		} else if (n == 0x4) {
			chip8_add(chip, ins);
		} else {
			fprintf(stderr, "Not Implemented: 0x%04X\n", ins);
			abort();
		}
	} else if (nibble_h == 0xA) {
		chip8_load_i(chip, ins);
	} else if (nibble_h == 0xC) {
		chip8_rnd(chip, ins);
	} else if (nibble_h == 0xD) {
		chip8_draw(chip, ins, renderer);
	} else if (nibble_h == 0xF) {
		y = ins & 0x00FF;
		if (y == 0x07) {
			chip8_load_from_dt(chip, ins);
		} else if (y == 0x0A) {
			chip8_waitkey(chip, ins);
		} else if (y == 0x15) {
			chip8_load_dt(chip, ins);
		} else if (y == 0x18) {
			chip8_load_st(chip, ins);
		} else if (y == 0x29) {
			chip8_load_i_hexfont(chip, ins);
		} else if (y == 0x33) {
			chip8_store_bcd(chip, ins);
		} else if (y == 0x65) {
			chip8_load_range_from_i(chip, ins);
		} else {
			fprintf(stderr, "Unrecognized instruction: 0x%04X\n",
				ins);
		}
	} else {
		fprintf(stderr, "Unrecognized instruction: 0x%04X\n", ins);
		abort();
	}

	return 0;
}

