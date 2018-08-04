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
	} else {
		c = str_to_addr(cmp);
	}
	return 0x3000 | ((v << 8) & 0x0F00) | (c & 0x00FF);
}
