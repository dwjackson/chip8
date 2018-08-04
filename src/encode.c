#include "encode.h"
#include "asm8.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define NOP 0x0000

static int is_label(const char *str, struct label labels[MAX_LABELS],
	size_t num_labels, unsigned short *addr_p);
static unsigned short encode_jump(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels);
static unsigned short encode_call(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels);
static unsigned short str_to_addr(const char *arg);
static unsigned short encode_se(struct statement *stmt);
static unsigned short encode_sne(struct statement *stmt);
static unsigned short encode_ld(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels);
static unsigned short encode_drw(struct statement *stmt);

unsigned short encode_statement(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels)
{
	unsigned short asm_stmt;
	char ins[INSTRUCTION_SIZE];

	if (!stmt->has_instruction) {
		return NO_INSTRUCTION;
	}

	strcpy(ins, stmt->instruction);
	size_t i;
	for (i = 0; i < strlen(ins); i++) {
		ins[i] = toupper(ins[i]);
	}

	if (strcmp(ins, "CLS") == 0) {
		asm_stmt = 0x00E0;
	} else if (strcmp(ins, "RET") == 0) {
		asm_stmt = 0x00EE;
	} else if (strcmp(ins, "JP") == 0) {
		asm_stmt = encode_jump(stmt, labels, num_labels);
	} else if (strcmp(ins, "CALL") == 0) {
		asm_stmt = encode_call(stmt, labels, num_labels);
	} else if (strcmp(ins, "SE") == 0) {
		asm_stmt = encode_se(stmt);
	} else if (strcmp(ins, "SNE") == 0) {
		asm_stmt = encode_sne(stmt);
	} else if (strcmp(ins, "LD") == 0) {
		asm_stmt = encode_ld(stmt, labels, num_labels);
	} else if (strcmp(ins, "DRW") == 0) {
		asm_stmt = encode_drw(stmt);
	} else {
		fprintf(stderr, "Unrecognized instruction: \"%s\"\n", ins);
		asm_stmt = NOP;
	}
	/* TODO */
	return asm_stmt;
}

static unsigned short encode_jump(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels)
{
	unsigned short addr;
	const char *arg;
	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for JP\n");
		abort();
	}
	arg = stmt->args[0];
	if (!is_label(arg, labels, num_labels, &addr)) {
		addr = str_to_addr(arg);
	}
	return 0x1000 | (addr & 0x0FFF);
}

static int is_label(const char *str, struct label labels[MAX_LABELS],
	size_t num_labels, unsigned short *addr_p)
{
	int is_label = 0;
	size_t i;
	const char *label;
	for (i = 0; i < num_labels; i++) {
		label = labels[i].text;
		if (strcmp(label, str) == 0) {
			is_label = 1;
			*addr_p = labels[i].addr;
		}
	}
	return is_label;
}

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
	struct label labels[MAX_LABELS], size_t num_labels)
{
	unsigned short addr;
	const char *arg;
	if (stmt->num_args < 1) {
		fprintf(stderr, "Too few arguments for CALL\n");
		abort();
	}
	arg = stmt->args[0];
	if (!is_label(arg, labels, num_labels, &addr)) {
		addr = str_to_addr(arg);
	}
	return 0x2000 | (addr & 0x0FFF);
}

static unsigned short encode_se(struct statement *stmt)
{
	const char *reg;
	const char *cmp;
	unsigned char v;
	unsigned char c;
	unsigned short high;
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
		high = 0x5000;
	} else {
		c = str_to_addr(cmp);
		high = 0x3000;
	}
	return high | ((v << 8) & 0x0F00) | (c & 0x00FF);
}

static unsigned short encode_sne(struct statement *stmt)
{
	const char *reg;
	const char *cmp;
	unsigned char v;
	unsigned char c;
	unsigned short high;
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
	struct label labels[MAX_LABELS], size_t num_labels)
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
	} else if (dst[0] == 'I') {
		high = 0xA000;
		if (!is_label(src, labels, num_labels, &addr)) {
			addr = str_to_addr(src);
		}
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
		src_byte = strtol(&src[1], NULL, 16);
		return high | ((dst_byte << 8) & 0x0F00) | (src_byte & 0x00FF); 
	} else {
		fprintf(stderr, "Unimplemented LD\n");
		print_statement(stmt);
		abort();
	}

	return NOP;
}

static unsigned short encode_drw(struct statement *stmt)
{
	unsigned short x;
	unsigned short y;
	unsigned short n;

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
