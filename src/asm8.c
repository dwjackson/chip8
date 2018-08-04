#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "asm8.h"
#include "fsm.h"
#include "encode.h"

#define USAGE_FMT "Usage: %s [FILE_NAME]\n"
#define BUFSIZE LINE_SIZE
#define TO_BIG_ENDIAN(s) ((((s) & 0x00FF) << 8) | (((s) & 0xFF00) >> 8))

void assemble(FILE *in_fp, FILE *out_fp);
void find_labels(FILE *in_fp, struct label labels[MAX_LABELS], size_t *num_labels); 
void parse_statement(char line[LINE_SIZE], struct statement *stmt);
void fsm_tick(struct fsm *fsm, struct statement *stmt, char buf[BUFSIZE], size_t *buf_len);
unsigned short statement_length(struct statement *stmt);
static void print_statement(struct statement *stmt);
void write_assembly(FILE *in_fp, FILE *out_fp, struct label labels[MAX_LABELS], size_t num_labels);

int main(int argc, char *argv[])
{
	char *in_file_name;
	FILE *in_fp;
	char out_file_name[] = "a.out";
	FILE *out_fp;

	if (argc < 2) {
		printf(USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}

	in_file_name = argv[1];
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

	return 0;
}

void assemble(FILE *in_fp, FILE *out_fp)
{
	struct label labels[MAX_LABELS];
	size_t num_labels = 0;

	find_labels(in_fp, labels, &num_labels);
	rewind(in_fp);
	write_assembly(in_fp, out_fp, labels, num_labels);
}

void find_labels(FILE *in_fp, struct label labels[MAX_LABELS], size_t *num_labels)
{
	char line[LINE_SIZE];
	struct statement stmt;
	unsigned short stmt_len;
	unsigned short addr = 0x0000;
	while (fgets(line, LINE_SIZE, in_fp) != NULL) {
		statement_reset(&stmt);
		parse_statement(line, &stmt);
		if (stmt.has_label) {
			strcpy(labels[*num_labels].text, stmt.label);
			labels[*num_labels].addr = addr;
			(*num_labels)++;
			if (*num_labels >= MAX_LABELS) {
				fprintf(stderr, "Too many labels\n");
				abort();
			}
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
			stmt->has_instruction = 1;
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
	if (stmt->has_instruction) {
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

static void print_statement(struct statement *stmt)
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

void write_assembly(FILE *in_fp, FILE *out_fp, struct label labels[MAX_LABELS], size_t num_labels)
{
	char line[LINE_SIZE];
	struct statement stmt;
	unsigned short asm_stmt;
	while (fgets(line, LINE_SIZE, in_fp) != NULL) {
		statement_reset(&stmt);
		parse_statement(line, &stmt);
		/*print_statement(&stmt); */ /* DEBUG */
		asm_stmt = encode_statement(&stmt, labels, num_labels);
		if (asm_stmt == NO_INSTRUCTION) {
			continue;
		}
		asm_stmt = TO_BIG_ENDIAN(asm_stmt);
		fwrite(&asm_stmt, 2, 1, out_fp);
	}
}

