/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#include "encode.h"
#include "asm8.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define NOP 0x0000
#define MNEMONIC_SIZE 5

typedef unsigned short (*encoder)(struct statement *stmt,
	struct assembler *assembler);

struct instruction {
	char mnemonic[MNEMONIC_SIZE];
	encoder encode;
	int bytes;
};

static unsigned short address_from(const char *str,
	struct assembler *assembler);
static int is_label(const char *str, struct assembler *assembler,
	unsigned short *addr_p);
static unsigned short str_to_addr(const char *arg);

static unsigned short encode_jump(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_call(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_se(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_sne(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_ld(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_drw(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_add(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_sub(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_sprite_byte(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_or(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_and(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_xor(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_bitwise(struct statement *stmt,
	unsigned short ins, const char *ins_name);
static unsigned short encode_skp(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_sknp(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_cls(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_ret(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_exit(struct statement *stmt,
	struct assembler *assembler);
static unsigned short encode_rnd(struct statement *stmt,
	struct assembler *assembler);

struct instruction instructions[] = {
	{ "ADD", encode_add, 2 },
	{ "AND", encode_and, 2 },
	{ "CALL", encode_call, 2 },
	{ "CLS", encode_cls, 2 },
	{ "DRW", encode_drw, 2 },
	{ "EXIT", encode_exit, 2 },
	{ "JP", encode_jump, 2 },
	{ "LD", encode_ld, 2 },
	{ "OR", encode_or, 2 },
	{ "RET", encode_ret, 2 },
	{ "RND", encode_rnd, 2 },
	{ "SE", encode_se, 2 },
	{ "SKNP", encode_sknp, 2 },
	{ "SKP", encode_skp, 2 },
	{ "SNE", encode_sne, 2 },
	{ "SUB", encode_sub, 2 },
	{ "XOR", encode_xor, 2 },
	{ ".SB", encode_sprite_byte, 1 }
};


int encode_statement(struct statement *stmt, struct assembler *assembler,
	unsigned short *asm_stmt_p)
{
	unsigned short asm_stmt;
	int bytes = 2;
	char ins[INSTRUCTION_SIZE];
	size_t i;
	size_t instr_count = sizeof(instructions) / sizeof(instructions[0]);

	if (!stmt->has_instruction) {
		return 0;
	}

	strcpy(ins, stmt->instruction);
	for (i = 0; i < strlen(ins); i++) {
		ins[i] = toupper(ins[i]);
	}

	bytes = 0;
	for (i = 0; i < instr_count; i++) {
		if (strcmp(instructions[i].mnemonic, ins) == 0) {
			asm_stmt = instructions[i].encode(stmt, assembler);
			bytes = instructions[i].bytes;
			break;
		}
	}
	if (bytes == 0)  {
		fprintf(stderr, "Unrecognized instruction: \"%s\"\n", ins);
		asm_stmt = NOP;
	}
	*asm_stmt_p = asm_stmt;
	return bytes;
}

static unsigned short encode_jump(struct statement *stmt,
	struct assembler *assembler)
{
	unsigned short addr;
	const char *arg;
	unsigned short head;
	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for JP\n");
		abort();
	}
	arg = stmt->args[0];
	head = 0x1000;
	if ((arg[0] == 'V' || arg[0] == 'v') && arg[1] == '0') {
		if (stmt->num_args < 2) {
			fprintf(stderr, "Too few arguments for JP V0\n");
			abort();
		}
		head = 0xB000;
		arg = stmt->args[1];
	}
	addr = address_from(arg, assembler);
	return head | (addr & 0x0FFF);
}

static unsigned short address_from(const char *str, struct assembler *assembler)
{
	unsigned short addr;
	if (is_label(str, assembler, &addr)) {
		return addr;
	}
	addr = str_to_addr(str);
	if (addr == 0x0000) {
		fprintf(stderr, "Invalid label/address: %s\n", str);
		abort();
	}
	return addr;
}

static int is_label(const char *str, struct assembler *assembler,
	unsigned short *addr_p)
{
	int is_label = 0;
	size_t i;
	const char *label;
	for (i = 0; i < assembler->num_labels; i++) {
		label = assembler->labels[i].text;
		if (strcmp(label, str) == 0) {
			is_label = 1;
			*addr_p = assembler->labels[i].addr;
		}
	}
	return is_label;
}

/*
 * Convert a string-encoded numerical address to an actual address
 * Return 0x0000 if the address is invalid
 */
static unsigned short str_to_addr(const char *arg)
{
	unsigned short addr;
	if (strstr(arg, "0x") == arg) {
		arg += 2;
		addr = strtol(arg, NULL, 16);
	} else {
		addr = strtol(arg, NULL, 10);
	}
	return addr;
}

static unsigned short encode_call(struct statement *stmt,
	struct assembler *assembler)
{
	unsigned short addr;
	const char *arg;
	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for CALL\n");
		abort();
	}
	arg = stmt->args[0];
	addr = address_from(arg, assembler);
	return 0x2000 | (addr & 0x0FFF);
}

static unsigned short encode_se(struct statement *stmt,
	struct assembler *assembler)
{
	const char *reg;
	const char *cmp;
	unsigned char v;
	unsigned char c;
	unsigned short high;

	(void) assembler;

	if (stmt->num_args < 2) {
		fprintf(stderr, "Too few arguments for SE\n");
		abort();
	}
	reg = stmt->args[0];
	cmp = stmt->args[1];
	if (!(reg[0] == 'V' || reg[0] == 'v')) {
		fprintf(stderr, "First argument to SE must be register\n");
		abort();
	}
	v = strtol(&reg[1], NULL, 16);
	if (cmp[0] == 'V' || cmp[0] == 'v') {
		c = str_to_addr(cmp);
		high = 0x5000;
	} else {
		c = str_to_addr(cmp);
		high = 0x3000;
	}
	return high | ((v << 8) & 0x0F00) | (c & 0x00FF);
}

static unsigned short encode_sne(struct statement *stmt,
	struct assembler *assembler)
{
	const char *reg;
	const char *cmp;
	unsigned char v;
	unsigned char c;
	unsigned short high;

	(void) assembler;

	if (stmt->num_args < 2) {
		fprintf(stderr, "Too few arguments for SE\n");
		abort();
	}
	reg = stmt->args[0];
	cmp = stmt->args[1];
	if (!(reg[0] == 'V' || reg[0] == 'v')) {
		fprintf(stderr, "First argument to SE must be register\n");
		abort();
	}
	v = strtol(&reg[1], NULL, 16);
	if (cmp[0] == 'V' || cmp[0] == 'v') {
		c = strtol(&cmp[1], NULL, 16);
		high = 0x9000;
	} else {
		c = str_to_addr(cmp);
		high = 0x4000;
	}
	return high | ((v << 8) & 0x0F00) | (c & 0x00FF);
}

static unsigned short encode_ld(struct statement *stmt,
	struct assembler *assembler)
{
	const char *dst;
	const char *src;
	unsigned short high;
	unsigned short dst_byte;
	unsigned short src_byte;
	unsigned short addr;

	if (stmt->num_args < 2) {
		fprintf(stderr, "Too few arguments for LD\n");
		abort();
	}

	dst = stmt->args[0];
	src = stmt->args[1];

	if ((dst[0] == 'V' || dst[0] == 'v')
		&& (src[0] == 'V' || src[0] == 'v')) {
		high = 0x8000;
		dst_byte = strtol(&dst[1], NULL, 16);
		src_byte = strtol(&src[1], NULL, 16);
		return high | ((dst_byte << 8) & 0x0F00)
			| (src_byte << 4 & 0x00F0) | 0x0000;
	} else if (dst[0] == 'I' || dst[0] == 'i') {
		high = 0xA000;
		addr = address_from(src, assembler);
		return high | (addr & 0x0FFF);
	} else if ((dst[0] == 'V' || dst[0] == 'v')
			&& (src[0] == 'd' || src[1] == 'D')
			&& (src[1] == 't' || src[1] == 'T')) {
		high = 0xF007;
		dst_byte = strtol(&dst[1], NULL, 16);
		return high | ((dst_byte << 8) & 0x0F00);
	} else if ((dst[0] == 'V' || dst[0] == 'v')
			&& (src[0] == 'k' || src[0] == 'K')) {
		high = 0xF00A;
		dst_byte = strtol(&dst[1], NULL, 16);
		return high | ((dst_byte << 8) & 0x0F00);
	} else if ((dst[0] == 'V' || dst[0] == 'v') 
			&& strcmp(src, "[I]") == 0) {
		high = 0xF065;
		dst_byte = strtol(&dst[1], NULL, 16);
		return high | ((dst_byte << 8) & 0x0F00);
	} else if (dst[0] == 'V' || dst[0] == 'v') {
		high = 0x6000;
		dst_byte = strtol(&dst[1], NULL, 16);
		src_byte = str_to_addr(src);
		return high | ((dst_byte << 8) & 0x0F00) | (src_byte & 0x00FF); 
	} else if ((dst[0] == 'D' || dst[0] == 'd')
			&& (dst[1] == 't' || dst[1] == 'T')) {
		high = 0xF015;
		dst_byte = strtol(&dst[1], NULL, 16);
		return high | ((dst_byte << 8) & 0x0F00);
	} else {
		fprintf(stderr, "Unimplemented LD\n");
		print_statement(stmt);
		abort();
	}

	return NOP;
}

static unsigned short encode_drw(struct statement *stmt,
	struct assembler *assembler)
{
	unsigned short x;
	unsigned short y;
	unsigned short n;

	(void) assembler;

	if (stmt->num_args < 3) {
		fprintf(stderr, "Too few arguments for DRW\n");
		abort();
	}

	x = strtol(&(stmt->args[0][1]), NULL, 16);
	y = strtol(&(stmt->args[1][1]), NULL, 16);
	n = str_to_addr(stmt->args[2]);

	return 0xD000
		| ((x << 8) & 0x0F00)
		| ((y << 4) & 0x00F0)
		| (n & 0x000F);
}

static unsigned short encode_add(struct statement *stmt,
	struct assembler *assembler)
{
	const char *dst;
	const char *src;
	unsigned short high;
	unsigned short dst_byte;
	unsigned short src_byte;
	unsigned short b;

	(void) assembler;

	if (stmt->num_args < 2) {
		fprintf(stderr, "Too few arguments for ADD\n");
		abort();
	}

	dst = stmt->args[0];
	src = stmt->args[1];

	if ((dst[0] == 'V' || dst[0] == 'v')
		&& (src[0] == 'V' || src[0] == 'v')) {
		high = 0x8004;
		dst_byte = strtol(&dst[1], NULL, 16);
		src_byte = strtol(&src[1], NULL, 16);
		return high
			| ((dst_byte << 8) & 0x0F00)
			| ((src_byte << 4) & 0x00F0);
	} else if (dst[0] == 'I' && (src[0] == 'V' || src[0] == 'v')) {
		high = 0xF01E;
		dst_byte = strtol(&dst[1], NULL, 16);
		return high | ((dst_byte << 8) & 0x0F00);
	} else if (dst[0] == 'V' || dst[0] == 'v') {
		dst_byte = strtol(&dst[1], NULL, 16);
		b = str_to_addr(src);
		high = 0x7000;
		return high | ((dst_byte << 8) & 0x0F00) | (b & 0x00FF);
	} else {
		fprintf(stderr, "Unimplemented ADD\n");
		abort();
	}

	return NOP;
}

static unsigned short encode_sprite_byte(struct statement *stmt,
	struct assembler *assembler)
{
	const char *arg;
	unsigned short b;

	(void) assembler;

	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for .SB\n");
		abort();
	}

	arg = stmt->args[0];
	b = str_to_addr(arg);

	return b;
}

static unsigned short encode_sub(struct statement *stmt,
	struct assembler *assembler)
{
	const char *dst;
	const char *src;
	unsigned short dst_byte;
	unsigned short src_byte;

	(void) assembler;

	if (stmt->num_args < 2) {
		fprintf(stderr, "Too few arguments for SUB\n");
		abort();
	}

	dst = stmt->args[0];
	src = stmt->args[1];
	dst_byte = strtol(&dst[1], NULL, 16);
	src_byte = strtol(&src[1], NULL, 16);

	return 0x8005 | ((dst_byte << 8) & 0x0F00) | ((src_byte << 4) & 0x00F0);
}

static unsigned short encode_or(struct statement *stmt,
	struct assembler *assembler)
{
	(void) assembler;
	return encode_bitwise(stmt, 0x8001, "OR");
}

static unsigned short encode_and(struct statement *stmt,
	struct assembler *assembler)
{
	(void) assembler;
	return encode_bitwise(stmt, 0x8002, "AND");
}

static unsigned short encode_xor(struct statement *stmt,
	struct assembler *assembler)
{
	(void) assembler;
	return encode_bitwise(stmt, 0x8003, "XOR");
}

static unsigned short encode_bitwise(struct statement *stmt,
	unsigned short ins, const char *ins_name)
{
	const char *dst;
	const char *src;
	unsigned short dst_byte;
	unsigned short src_byte;

	if (stmt->num_args < 2) {
		fprintf(stderr, "Too few arguments for %s\n", ins_name);
		abort();
	}

	dst = stmt->args[0];
	src = stmt->args[1];
	dst_byte = strtol(&dst[1], NULL, 16);
	src_byte = strtol(&src[1], NULL, 16);

	return ins | ((dst_byte << 8) & 0x0F00) | ((src_byte << 4) & 0x00F0);
}

static unsigned short encode_skp(struct statement *stmt,
	struct assembler *assembler)
{
	const char *reg;
	unsigned char b;

	(void) assembler;

	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for SKP\n");
		abort();
	}

	reg = stmt->args[0];
	b = strtol(&reg[1], NULL, 16);

	return 0xE09E | ((b << 8) & 0x0F00);
}

static unsigned short encode_sknp(struct statement *stmt,
	struct assembler *assembler)
{
	const char *reg;
	unsigned char b;

	(void) assembler;

	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for SKNP\n");
		abort();
	}

	reg = stmt->args[0];
	b = strtol(&reg[1], NULL, 16);

	return 0xE0A1 | ((b << 8) & 0x0F00);
}

static unsigned short encode_cls(struct statement *stmt,
	struct assembler *assembler)
{
	(void) stmt;
	(void) assembler;
	return 0x00E0;
}

static unsigned short encode_ret(struct statement *stmt,
	struct assembler *assembler)
{
	(void) stmt;
	(void) assembler;
	return 0x00EE;
}

static unsigned short encode_exit(struct statement *stmt,
	struct assembler *assembler)
{
	(void) stmt;
	(void) assembler;

	return 0x00FD;
}

static unsigned short encode_rnd(struct statement *stmt,
	struct assembler *assembler)
{
	const char *reg;
	unsigned char v;
	unsigned short b;

	(void) assembler;

	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for RND\n");
		abort();
	}

	reg = stmt->args[0];
	if (!(reg[0] == 'V' || reg[0] == 'v')) {
		fprintf(stderr, "First argument to RND must be register\n");
		abort();
	}
	v = strtol(&reg[1], NULL, 16);
	b = str_to_addr(stmt->args[1]);

	return 0xC000 | ((v << 8) & 0x0F00) | (b & 0x00FF);
}
