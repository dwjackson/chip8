/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"

#define NIBBLE_H(b) (((b) & 0xF0) >> 4)
#define NIBBLE_L(b) ((b) & 0x0F)
#define ADDR(ins) ((ins) & 0x0FFF)

typedef unsigned char byte;
typedef unsigned short instruction;

void disassemble(FILE *fp);
void decode_8(instruction ins);
void decode_e(instruction ins);
void decode_f(instruction ins);

int main(int argc, char *argv[])
{
	FILE *fp;
	char *file_name;

	if (argc < 2) {
		fp = stdin;
	} else {
		file_name = argv[1];
		fp = fopen(file_name, "rb");
		if (!fp) {
			perror(file_name);
			exit(EXIT_FAILURE);
		}
	}
	disassemble(fp);
	fclose(fp);
	return 0;
}

void disassemble(FILE *fp)
{
	int ins_addr;
	instruction ins;
	byte high;
	byte low;
	byte nibble_h;
	unsigned short addr;
	byte x;
	byte y;
	unsigned short nibble;

	/* Note: 16 8-bit registers */
	ins_addr = CHIP8_PROGSTART;
	while (!feof(fp)) {
		if (fread(&ins, 1, 2, fp) < 2) {
			ins = ins & 0x00FF;
			printf("0x%04X: %02X\n", ins_addr, ins);
			continue;
		}	
		ins = TO_BIG_ENDIAN(ins);
		high = ins >> 8;
		low = ins & 0x00FF;
		nibble_h = NIBBLE_H(high);

		printf("0x%04X: %02X %02X\t", ins_addr, high, low);
		if (nibble_h == 0x0) {
			byte x = ins & 0x00FF;
			if (x == 0xEE) {
				printf("RET\n");
			} else if (x == 0xE0) {
				printf("CLS\n");
			} else if (x == 0x00) {
				printf("NOP\n");
			} else if (x == 0xFD) {
				printf("EXIT\n");
			} else {
				printf("0x%04x\n", ins);
			}
		} else if (nibble_h == 0x6) {
			printf("LD V%X, 0x%02X\n", NIBBLE_L(high), low);
		} else if (nibble_h == 0xA) {
			addr = ADDR(ins);
			printf("LD I, 0x%04X\n", addr);
		} else if (nibble_h == 0xD) {
			x = (ins & 0x0F00) >> 8;
			y = (ins & 0x00F0) >> 4;
			nibble = ins & 0x000F;
			printf("DRW V%X, V%X, %X\n", x, y, nibble);
		} else if (nibble_h == 0x2) {
			addr = ADDR(ins);
			printf("CALL 0x%02X\n", addr);
		} else if (nibble_h == 0xF) {
			decode_f(ins);
		} else if (nibble_h == 0x3) {
			x = (ins & 0x0F00) >> 8;
			y = ins & 0x00FF;
			printf("SE V%X, 0x%02X\n", x, y);
		} else if (nibble_h == 0x1) {
			addr = ADDR(ins);
			printf("JP 0x%04X\n", addr);
		} else if (nibble_h == 0xC) {
			x = (ins & 0x0F00) >> 16;
			y = ins & 0x00FF;
			printf("RND V%X, 0x%02X\n", x, y);
		} else if (nibble_h == 0xE) {
			decode_e(ins);
		} else if (nibble_h == 0x7) {
			x = (ins & 0x0F00) >> 16;
			y = ins & 0x00FF;
			printf("ADD V%X, 0x%02X\n", x, y);
		} else if (nibble_h == 0x8) {
			decode_8(ins);
		} else if (nibble_h == 0x4) {
			x = ins & 0x0F00 >> 8;
			y = ins & 0x00FF;
			printf("SNE V%X, 0x%02x\n", x, y);
		} else {
			printf("0x%02x\n", ins);
		}
		ins_addr += 2;
	}
}

void decode_f(instruction ins)
{
	byte x;
	byte arg;

	x = (ins & 0x0F00) >> 8;
	arg = ins & 0x00FF;

	if (arg == 0x07) {
		printf("LD V%X, DT\n", x);
	} else if (arg == 0x0A) {
		printf("LD V%X, K\n", x);
	} else if (arg == 0x15) {
		printf("LD DT, V%X\n", x);
	} else if (arg == 0x18) {
		printf("LD ST, V%X\n", x);
	} else if (arg == 0x29) {
		printf("LD F, V%X\n", x);
	} else if (arg == 0x33) {
		printf("LD B, V%X\n", x);
	} else if (arg == 0x55) {
		printf("LD [I], V%x\n", x);
	} else if (arg == 0x65) {
		printf("LD V%X, [I]\n", x);
	} else {
		printf("0x%x\n", ins);
	}
}

void decode_e(instruction ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte arg = ins & 0x00FF;
	if (arg == 0x9E) {
		printf("SKP V%X\n", x);
	} else if (arg == 0xA1) {
		printf("SKNP V%X\n", x);
	} else {
		printf("Invalid E: 0x%04X\n", ins);
	}
}

void decode_8(instruction ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	byte arg = ins & 0x000F;

	if (arg == 0x0) {
		printf("LD V%X, V%X\n", x, y);
	} else if (arg == 0x1) {
		printf("OR V%X, V%X\n", x, y);
	} else if (arg == 0x2) {
		printf("AND V%X, V%X\n", x, y);
	} else if (arg == 0x3) {
		printf("XOR V%X, V%X\n", x, y);
	} else if (arg == 0x4) {
		printf("ADD V%X, V%X\n", x, y);
	} else if (arg == 0x5) {
		printf("SUB V%X, V%X\n", x, y);
	} else if (arg == 0x6) {
		printf("SHR V%X, {, V%X}\n", x, y);
	} else if (arg == 0x7) {
		printf("SUBN V%X, V%X\n", x, y);
	} else if (arg == 0xE) {
		printf("SHL V%X, {, V%X}\n", x, y);
	} else {
		printf("TODO: 0x%04X\n", ins);
	}
}
