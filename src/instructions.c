#include "instructions.h"
#include <stdio.h>
#include <stdlib.h>

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

/* 3xkk - skip next instruction if Vx = kk */
void chip8_sne(struct chip8 *chip, unsigned short ins)
{
	byte x, y;
	byte regval;

	/* SE Vx, byte */
	x = (ins & 0x0F00) >> 8;
	y = ins & 0x00FF;
	regval = chip->reg_v[x];
	if (regval == y) {
		chip->pc += 2;
	}
}

void chip8_load_i(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr;
	/* LD I, addr */
	addr = ins & 0x0FFF;
	chip->reg_i = addr;
}
