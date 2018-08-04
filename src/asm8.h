/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#ifndef ASM8_H
#define ASM8_H

#define LINE_SIZE 100
#define LABEL_SIZE LINE_SIZE 
#define MAX_LABELS 100
#define INSTRUCTION_SIZE LINE_SIZE 
#define ARG_SIZE LINE_SIZE 
#define MAX_ARGS 3

struct label {
	unsigned short addr;
	char text[LABEL_SIZE];
};

struct statement {
	char label[LABEL_SIZE];
	int has_label;
	char instruction[INSTRUCTION_SIZE];
	int has_instruction;
	char args[MAX_ARGS][ARG_SIZE];
	int num_args;
};

void statement_reset(struct statement *stmt);
void print_statement(struct statement *stmt);

#endif /* ASM8_H */
