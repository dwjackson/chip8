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
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "asm8.h"
#include "fsm.h"
#include "encode.h"
#include "chip8.h"

#define USAGE_FMT "Usage: %s [FILE_NAME]\n"
#define BUFSIZE LINE_SIZE
#define DEFAULT_OUT_FILE_NAME "a.out"

static int label_exists(struct assembler *assembler, const char *label);

void assemble(FILE *in_fp, FILE *out_fp);
void find_labels(FILE *in_fp, struct assembler *assembler);
void parse_statement(char line[LINE_SIZE], struct statement *stmt);
void fsm_tick(struct fsm *fsm, struct statement *stmt, char buf[BUFSIZE], size_t *buf_len);
unsigned short statement_length(struct statement *stmt);
void print_statement(struct statement *stmt);
void write_assembly(FILE *in_fp, FILE *out_fp, struct assembler *assembler);
static void print_labels(struct label labels[MAX_LABELS], size_t num_labels);

int main(int argc, char *argv[])
{
	char *in_file_name;
	FILE *in_fp;
	char *out_file_name;
	FILE *out_fp;
	extern char *optarg;
	extern int optind;
	int opt;

	out_file_name = NULL;
	while ((opt = getopt(argc, argv, "o:")) > 0) {
		switch (opt) {
		case 'o':
			out_file_name = malloc(strlen(optarg) + 1);
			strcpy(out_file_name, optarg);
			break;
		default:
			/* Do nothing */
			break;
		}
	}
	if (out_file_name == NULL) {
		out_file_name = malloc(strlen(DEFAULT_OUT_FILE_NAME) + 1);
		strcpy(out_file_name, DEFAULT_OUT_FILE_NAME);
	}

	if (argc < 2) {
		printf(USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}

	in_file_name = argv[optind];
	in_fp = fopen(in_file_name, "r");
	if (!in_fp) {
		perror(in_file_name);
		exit(EXIT_FAILURE);
	}

	out_fp = fopen(out_file_name, "wb");
	if (!out_fp) {
		perror(out_file_name);
		fclose(in_fp);
		exit(EXIT_FAILURE);
	}

	assemble(in_fp, out_fp);
	fclose(in_fp);
	free(out_file_name);

	return 0;
}

void assemble(FILE *in_fp, FILE *out_fp)
{
	struct assembler assembler;
	assembler.num_labels = 0;

	find_labels(in_fp, &assembler);
	/* print_labels(labels, num_labels); */ /* DEBUG */
	rewind(in_fp);
	write_assembly(in_fp, out_fp, &assembler);
}

void find_labels(FILE *in_fp, struct assembler *assembler)
{
	char line[LINE_SIZE];
	struct statement stmt;
	unsigned short stmt_len;
	unsigned short addr = CHIP8_PROGSTART;
	int label_already_exists;
	while (fgets(line, LINE_SIZE, in_fp) != NULL) {
		statement_reset(&stmt);
		parse_statement(line, &stmt);
		label_already_exists = label_exists(assembler, stmt.label);
		if (stmt.has_label && !label_already_exists) {
			strcpy(assembler->labels[assembler->num_labels].text,
				stmt.label);
			assembler->labels[assembler->num_labels].addr = addr;
			assembler->num_labels++;
			if (assembler->num_labels >= MAX_LABELS) {
				fprintf(stderr, "Too many labels\n");
				abort();
			}
		} else if (label_already_exists) {
			fprintf(stderr, "Label already exists: %s\n",
				stmt.label);
		}
		stmt_len = statement_length(&stmt);
		addr += stmt_len;
		/*print_statement(&stmt); */ /* DEBUG */
	}
}

void parse_statement(char line[LINE_SIZE], struct statement *stmt)
{
	struct fsm fsm;
	char buf[BUFSIZE];
	size_t buf_len;

	buf_len = 0;
	memset(buf, 0, BUFSIZE);
	fsm_init(&fsm, line, stmt);

	while (!fsm_is_done(&fsm)) {
		fsm_tick(&fsm, stmt, buf, &buf_len);
	}	
}

void fsm_tick(struct fsm *fsm, struct statement *stmt, char buf[BUFSIZE], size_t *buf_len)
{
	char nextch;
	enum state currstate;
	enum state nextstate;

	nextch = fsm_nextchar(fsm);
	nextstate = fsm_nextstate(fsm, nextch);
	currstate = fsm->currstate;
	if (currstate != nextstate) {
		switch(currstate) {
		case STATE_START:
			/* Do nothing */
			break;
		case STATE_LABEL:
			strcpy(stmt->label, buf);
			stmt->has_label = 1;
			break;
		case STATE_AFTER_LABEL:
			/* Do nothing */
			break;
		case STATE_INSTRUCTION:
			strcpy(stmt->instruction, buf); 
			if (strlen(stmt->instruction) > 0) {
				stmt->has_instruction = 1;
			} else {
				stmt->has_instruction = 0;
			}
			break;
		case STATE_WHITESPACE:
			/* Do nothing */
			break;
		case STATE_ARGUMENT:
			if (stmt->num_args > MAX_ARGS) {
				fprintf(stderr, "Too many arguments\n");
				abort();
			}
			strcpy(stmt->args[stmt->num_args], buf);
			stmt->num_args++;
			break;
		case STATE_COMMENT:
			/* Do nothing */
			break;
		case STATE_DONE:
			/* Do nothing */
			break;
		default:
			fprintf(stderr, "Invalid state: %d\n", currstate);
			abort();
		}

		/* Reset buffer */
		*buf_len = 0;
		memset(buf, 0, BUFSIZE);

		fsm_transition(fsm, nextstate);
	}
	buf[*buf_len] = nextch;
	(*buf_len)++;
	if (*buf_len >= BUFSIZE) {
		fprintf(stderr, "Input buffer overflow\n");
		abort();
	}
}

unsigned short statement_length(struct statement *stmt)
{
	char ins[INSTRUCTION_SIZE];
	size_t i;
	if (stmt->has_instruction) {
		strcpy(ins, stmt->instruction);
		for (i = 0; i < strlen(ins); i++) {
			ins[i] = toupper(ins[i]);
		}
		if (strcmp(ins, ".SB") == 0) {
			return 1;
		}
		return 2;
	}
	return 0;
}

void statement_reset(struct statement *stmt)
{
	int i;

	memset(stmt->label, 0, LABEL_SIZE);
	stmt->has_label = 0;
	memset(stmt->instruction, 0, INSTRUCTION_SIZE);
	stmt->has_instruction = 0;
	for (i = 0; i < MAX_ARGS; i++) {
		memset(stmt->args[i], 0, ARG_SIZE);
	}
	stmt->num_args = 0;
}

void print_statement(struct statement *stmt)
{
	unsigned short stmt_len = statement_length(stmt);
	printf("LABEL: \"%s\"\n", stmt->label);
	printf("INSTRUCTION: \"%s\"\n", stmt->instruction);
	int i;
	for (i = 0; i < stmt->num_args; i++) {
		printf("arg[%d] = \"%s\"\n", i, stmt->args[i]);
	}
	printf("statement length = %hd\n", stmt_len);
}

void write_assembly(FILE *in_fp, FILE *out_fp, struct assembler *assembler)
{
	char line[LINE_SIZE];
	struct statement stmt;
	unsigned short asm_stmt;
	int bytes;
	unsigned char b;

	while (fgets(line, LINE_SIZE, in_fp) != NULL) {
		statement_reset(&stmt);
		parse_statement(line, &stmt);
		/*print_statement(&stmt); */ /* DEBUG */
		bytes = encode_statement(&stmt, assembler, &asm_stmt);
		if (bytes == 0) {
			continue;
		}
		if (bytes == 2) {
			asm_stmt = TO_BIG_ENDIAN(asm_stmt);
			fwrite(&asm_stmt, 2, 1, out_fp);
		} else if (bytes == 1) {
			b = asm_stmt;
			fwrite(&b, 1, 1, out_fp);
		}
	}
}

static void print_labels(struct label labels[MAX_LABELS], size_t num_labels)
{
	size_t i;
	struct label *lbl;
	for (i = 0; i < num_labels; i++) {
		lbl = &(labels[i]);
		printf("%s: %04X\n", lbl->text, lbl->addr);
	}
}

static int label_exists(struct assembler *assembler, const char *label)
{
	size_t i;
	const struct label *existing_label;
	for (i = 0; i < assembler->num_labels; i++) {
		existing_label = &(assembler->labels[i]);
		if (strcmp(existing_label->text, label) == 0) {
			return 1;
		}
	}
	return 0;
}
