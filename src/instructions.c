/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#include "instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"

static int chip8_pushpc(struct chip8 *chip)
{
	if (chip->sp + 1 > CHIP8_STACKSIZE) {
		return 1;
	}
	chip->sp++;
	chip->stack[chip->sp] = chip->pc;
	return 0;
}

static int chip8_poppc(struct chip8 *chip)
{
	if (chip->sp - 1 < 0) {
		return 1;
	}
	chip->pc = chip->stack[chip->sp];
	chip->sp--;
	return 0;
}

void chip8_ret(struct chip8 *chip)
{
	chip8_poppc(chip);
}

void chip8_call(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr;

	/* CALL addr */
	addr = ins & 0x0FFF;
	chip8_pushpc(chip);
	chip->pc = addr;
}

void chip8_jump(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr;

	/* JP addr */
	addr = ins & 0x0FFF;
	if (addr > CHIP8_RAMBYTES) {
		fprintf(stderr, "Invalid jump\n");
		abort();
	}
	chip->pc = addr;
}

static void skip_next(struct chip8 *chip)
{
	chip->pc += 2;
}

/* 3xkk - skip next instruction if Vx = kk */
void chip8_se_immediate(struct chip8 *chip, unsigned short ins)
{
	byte x, y;
	byte regval;

	/* SE Vx, byte */
	x = (ins & 0x0F00) >> 8;
	y = ins & 0x00FF;
	regval = chip->reg_v[x];
	if (regval == y) {
		skip_next(chip);
	}
}

/* 4xkk - Skip next instruction if Vx == kk */
void chip8_sne_immediate(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte kk = ins & 0x00FF;
	byte regval = chip->reg_v[x];
	if (regval != kk) {
		skip_next(chip);
	}
}

/* 9xy0 - Skip next instruction if Vx == Vy */
void chip8_sne(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	if (chip->reg_v[x] != chip->reg_v[y]) {
		skip_next(chip);
	}
}

void chip8_se(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	if (chip->reg_v[x] == chip->reg_v[y]) {
		skip_next(chip);
	}
}

void chip8_load_i(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr;
	/* LD I, addr */
	addr = ins & 0x0FFF;
	chip->reg_i = addr;
}

void chip8_load_immediate(struct chip8 *chip, unsigned short ins)
{
	byte x, y;

	/* LD Vx, byte */
	x = (ins & 0x0F00) >> 8;
	y = ins & 0x00FF;
	chip8_setv(chip, x, y);
}

static void print_sprite(byte *sprite, int n)
{
	int i, j;
	int bit;
	for (i = 0; i < n; i++) {
		for (j = 7; j >= 0; j--) {
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

static int is_within_display(int x, int y)
{
	return x >= 0 && y >= 0 && x < CHIP8_DISPLAYW && y < CHIP8_DISPLAYH;
}

/* DRW Vx, Vy, byte */
void chip8_draw(struct chip8 *chip, unsigned short ins)
{
	byte x, y;
	byte n; /* Sprite length */
	unsigned short addr;
	byte vx, vy;
	byte sprite[CHIP8_SPRITEBYTES];
	int i, j;
	int bit;
	int oldbit;
	int currbit;
	int disp_x, disp_y;
	byte collision;

	x = (ins & 0x0F00) >> 8;
	y = (ins & 0x00F0) >> 4;
	n = ins & 0x000F;
	if (n > CHIP8_SPRITEBYTES) {
		fprintf(stderr, "Invalid sprite length: 0x%04X\n", ins);
		exit(EXIT_FAILURE);
	}

	addr = chip->reg_i;
	if (addr >= CHIP8_RAMBYTES) {
		fprintf(stderr, "Invalid draw address: 0x%04X\n", addr);
		abort();
	}

	vx = chip->reg_v[x];
	vy = chip->reg_v[y];

	for (i = 0; i < n; i++) {
		sprite[i] = chip->ram[addr + i];
	}
	/* print_sprite(sprite, n); */ /* DEBUG */

	oldbit = 0x0;
	currbit = 0x0;
	collision = 0x0;
	for (i = 0; i < n; i++) {
		for (j = 7; j >= 0; j--) {
			disp_x = vx + (7 - j);
			disp_y = vy + i;
			if (!is_within_display(disp_x, disp_y)) {
				continue;
			}
			bit = BIT(sprite[i], j);
			oldbit = chip8_getpixel(chip, disp_x, disp_y);
			currbit = bit ^ oldbit;
			chip8_setpixel(chip, disp_x, disp_y, currbit);
			if (collision == 0x0) {
				collision = oldbit == 0x1 && currbit == 0x0;
			}
		}
	}
	chip8_setvf(chip, collision);
}

void chip8_add_immediate(struct chip8 *chip, unsigned short ins)
{
	byte x, y;

	/* ADD Vx, byte */
	x = (ins & 0x0F00) >> 8;
	y = ins & 0x00FF;
	chip8_setv(chip, x, chip->reg_v[x] + y);
}

void chip8_add(struct chip8 *chip, unsigned short ins)
{
	byte x, y;
	unsigned int result;

	x = (ins & 0x0F00) >> 8;
	y = (ins & 0x00F0) >> 4;

	/* ADD Vx, Vy */
	result = chip->reg_v[x] + chip->reg_v[y];
	if (result > 0xFF) {
		chip8_setvf(chip, 0x1);
	} else {
		chip8_setvf(chip, 0x0);
	}
	chip8_setv(chip, x, result & 0xFF);
}

void chip8_sub(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	if (chip->reg_v[x] > chip->reg_v[y]) {
		chip8_setvf(chip, 0x1);
	} else {
		chip8_setvf(chip, 0x0);
	}
	chip8_setv(chip, x, chip->reg_v[x] - chip->reg_v[y]);
}

void chip8_cls(struct chip8 *chip)
{
	int i, j;
	for (i = 0; i < CHIP8_DISPLAYH; i++) {
		for (j = 0; j < CHIP8_DISPLAYW; j++) {
			chip8_setpixel(chip, j, i, 0x0);
		}
	}
}

void chip8_ld(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	chip8_setv(chip, x, chip->reg_v[y]);
}

void chip8_waitkey(struct chip8 *chip, unsigned short ins)
{
	byte x;
	byte keycode;

	/* LD Vx, K */
	x = (ins & 0x0F00) >> 8;
	keycode = chip->keyboard->waitkey();
	chip8_setv(chip, x, keycode);
}

void chip8_load_from_dt(struct chip8* chip, unsigned short ins)
{
	/* LD Vx, DT */
	byte x = (ins & 0x0F00) >> 8;
	chip8_setv(chip, x, chip->reg_dt);
}

void chip8_load_dt(struct chip8 *chip, unsigned short ins)
{
	/* LD DT, Vx */
	byte x = (ins & 0x0F00) >> 8;
	chip->reg_dt = chip->reg_v[x];
}

void chip8_load_st(struct chip8 *chip, unsigned short ins)
{
	/* LD ST, Vx */
	byte x = (ins & 0x0F00) >> 8;
	chip->reg_st = chip->reg_v[x];
}

void chip8_load_range_from_i(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	int i;
	unsigned short addr;

	if (x > 0xE) {
		x = 0xE;
	}

	/* LD Vx, [I] */
	for (i = 0; i <= x; i++) {
		addr = chip->reg_i + i;
		if (addr > CHIP8_RAMBYTES || addr < CHIP8_PROGSTART) {
			fprintf(stderr, "Invalid memory access\n");
			abort();
		}
		chip8_setv(chip, i, chip->ram[addr]);
	}
}

void chip8_store_bcd(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte val = chip->reg_v[x];
	unsigned short addr = chip->reg_i;
	byte hundreds = val / 100;
	byte tens = (val - hundreds * 100) / 10;
	byte ones = val - hundreds * 100 - tens * 10;

	if (addr + 2 >= CHIP8_RAMBYTES) {
		fprintf(stderr, "RAM overflow\n");
		abort();
	}

	chip->ram[addr] = hundreds;
	chip->ram[addr + 1] = tens;
	chip->ram[addr + 2] = ones;
}

void chip8_load_i_hexfont(struct chip8 *chip, unsigned short ins)
{
	byte x;
	byte val;

	/* LD F, Vx */
	x = (ins & 0x0F00) >> 8;
	val = chip->reg_v[x];
	if (val <= 0xF) {
		chip->reg_i = CHIP8_FONTSTART + val * CHIP8_FONTWIDTH;
	}
}

void chip8_rnd(struct chip8 *chip, unsigned short ins)
{
	/* RND Vx, byte */
	byte x = (ins & 0x0F00) >> 8;
	byte b = ins & 0x00FF;
	byte r = rand();
	chip8_setv(chip, x, r & b);
}

void chip8_or(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	chip8_setv(chip, x, chip->reg_v[x] | chip->reg_v[y]);
}

void chip8_and(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	chip8_setv(chip, x, chip->reg_v[x] & chip->reg_v[y]);
}

void chip8_skp(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte keycode = chip->reg_v[x];
	if (chip->keyboard->is_key_down(keycode)) {
		skip_next(chip);
	}
}

void chip8_sknp(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte keycode = chip->reg_v[x];
	if (!chip->keyboard->is_key_down(keycode)) {
		skip_next(chip);
	}
}

void chip8_shr(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	if ((chip->reg_v[x] & 0x1) == 0x1) {
		chip8_setvf(chip, 0x1);
	} else {
		chip8_setvf(chip, 0x0);
	}
	chip8_setv(chip, x, chip->reg_v[x] >> 1);
}

void chip8_subn(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	if (chip->reg_v[y] > chip->reg_v[x]) {
		chip8_setvf(chip, 0x1);
	} else {
		chip8_setvf(chip, 0x0);
	}
	chip8_setv(chip, x, chip->reg_v[y] - chip->reg_v[x]);
}

void chip8_shl(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	if ((chip->reg_v[x] & 0x1) == 0x1) {
		chip8_setvf(chip, 0x1);
	} else {
		chip8_setvf(chip, 0x0);
	}
	chip8_setv(chip, x, chip->reg_v[x] << 1);
}

void chip8_jump_add(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr = ins & 0x0FFF;
	unsigned short result_addr = addr + chip->reg_v[0];
	if (result_addr > CHIP8_RAMBYTES || result_addr < CHIP8_PROGSTART) {
		fprintf(stderr, "Invalid memory access\n");
		abort();
	}
	chip->pc = result_addr;
}

void chip8_store_range_from_i(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	int i;
	unsigned short addr;

	if (x > 0xF) {
		x = 0xF;
	}

	for (i = 0; i <= x; i++) {
		addr = chip->reg_i + i;
		if (addr > CHIP8_RAMBYTES || addr < CHIP8_PROGSTART) {
			fprintf(stderr, "Memory access error\n");
			abort();
		}
		chip->ram[addr] = chip->reg_v[i];
	}
}
