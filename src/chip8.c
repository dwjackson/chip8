/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#include "chip8.h"
#include "instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOAD_BUFSIZE 512

void chip8_init(struct chip8 *chip, struct chip8_keyboard *keyboard,
	struct chip8_renderer *renderer,
	void (*check_kill)(struct chip8 *chip))
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
	chip->reg_i = 0x00;
	chip->reg_dt = 0x0;
	chip->reg_st = 0x00;
	memset(chip->ram, 0, CHIP8_RAMBYTES);
	chip->pc = 0;
	chip->sp = 0;
	memset(chip->stack, 0, CHIP8_STACKSIZE * sizeof(unsigned short));
	for (i = 0 ; i < CHIP8_DISPLAYH; i++) {
		for (j = 0; j < CHIP8_DISPLAYW; j++) {
			chip8_setpixel(chip, j, i, 0x0);
		}
	}
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 5; j++) {
			addr = CHIP8_FONTSTART + i * CHIP8_FONTWIDTH + j;
			chip->ram[addr] = fontchars[i][j];
		}
	}
	chip->keyboard = keyboard;
	chip->renderer = renderer;
	chip->is_halted = 0;
	chip->check_kill = check_kill;
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
		perror(file_name);
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

void chip8_exec(struct chip8 *chip)
{
	unsigned short ins;

	chip->pc = CHIP8_PROGSTART;
	while (chip->pc + 2 < CHIP8_RAMBYTES && !chip->is_halted) {
		ins = (*(unsigned short *)&((chip->ram[chip->pc])));
		ins = TO_BIG_ENDIAN(ins);
		chip->pc += 2;
		if (chip8_decode(chip, ins) != 0) {
			break;
		}
		chip->renderer->render_display(chip);
		chip->check_kill(chip);
	}
	chip8_halt(chip);
}

int chip8_decode(struct chip8 *chip, unsigned short ins)
{
	byte nibble_h;
	byte y;
	byte n;

	nibble_h = (ins & 0xF000) >> 12;
	if (nibble_h == 0x0) {
		y = ins & 0x00FF;
		if (y == 0x00) {
			/* NOP */
		} else if (y == 0xE0) {
			chip8_cls(chip);
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
		chip8_se_immediate(chip, ins);
	} else if (nibble_h == 0x4) {
		chip8_sne_immediate(chip, ins);
	} else if (nibble_h == 0x5) {
		chip8_se(chip, ins);
	} else if (nibble_h == 0x6) {
		chip8_load_immediate(chip, ins);
	} else if (nibble_h == 0x7) {
		chip8_add_immediate(chip, ins);
	} else if (nibble_h == 0x8) {
		n = ins & 0x000F;
		if (n == 0x0) {
			chip8_ld(chip, ins);
		} else if (n == 0x1) {
			chip8_or(chip, ins);
		} else if (n == 0x2) {
			chip8_and(chip, ins);
		} else if (n == 0x4) {
			chip8_add(chip, ins);
		} else if (n == 0x5) {
			chip8_sub(chip, ins);
		} else if (n == 0x6) {
			chip8_shr(chip, ins);
		} else if (n == 0x7) {
			chip8_subn(chip, ins);
		} else if (n == 0xE) {
			chip8_shl(chip, ins);
		} else {
			fprintf(stderr, "Not Implemented: 0x%04X\n", ins);
			abort();
		}
	} else if (nibble_h == 0x9) {
		chip8_sne(chip, ins);
	} else if (nibble_h == 0xA) {
		chip8_load_i(chip, ins);
	} else if (nibble_h == 0xB) {
		chip8_jump_add(chip, ins);
	} else if (nibble_h == 0xC) {
		chip8_rnd(chip, ins);
	} else if (nibble_h == 0xD) {
		chip8_draw(chip, ins);
	} else if (nibble_h == 0xE) {
		n = (ins & 0x00FF);
		if (n == 0x9E) {
			chip8_skp(chip, ins);
		} else if (n == 0xA1) {
			chip8_sknp(chip, ins);
		} else {
			fprintf(stderr, "Bad instruction: 0x%04X\n", ins);
			abort();
		}
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
		} else if (y == 0x55) {
			chip8_store_range_from_i(chip, ins);
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

int chip8_setv(struct chip8 *chip, byte index, byte value)
{
	if (index > 0xF) {
		return -1;
	}
	chip->reg_v[index] = value;
	return 0;
}

void chip8_setvf(struct chip8 *chip, byte val)
{
	if (val > 1) {
		val = 0x1;
	}
	chip->reg_v[0xF] = val;
}

void chip8_setpixel(struct chip8 *chip, byte x, byte y, byte val)
{
	chip->display[y][x] = val;
}

byte chip8_getpixel(struct chip8 *chip, byte x, byte y)
{
	return chip->display[y][x];
}

void chip8_halt(struct chip8 *chip)
{
	chip->is_halted = 1;
}
