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
#include "chip8.h"

#define USAGE_FMT "Usage: %s [FILE_NAME]\n"
#define OUTFILE_DEFAULTNAME "a.rom"
#define LINE_SIZE 100

void assemble(FILE *in, FILE *out);

int main(int argc, char *argv[])
{
	char *file_name;
	FILE *in_fp;
	FILE *out_fp;

	if (argc < 2) {
		printf(USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}

	file_name = argv[1];
	in_fp = fopen(file_name, "r");
	if (!in_fp) {
		perror(file_name);
		exit(EXIT_FAILURE);
	}
	out_fp = fopen(OUTFILE_DEFAULTNAME, "wb");
	if (!out_fp) {
		perror(OUTFILE_DEFAULTNAME);
		exit(EXIT_FAILURE);
	}

	assemble(in_fp, out_fp);

	fclose(out_fp);
	fclose(in_fp);

	return 0;
}

void assemble(FILE *in, FILE *out)
{
	char line[LINE_SIZE];
	unsigned short x;
	unsigned short val;
	unsigned short ins; /* instruction */

	if (fgets(line, LINE_SIZE, in) == NULL) {
		perror("fgets");
		exit(EXIT_FAILURE);
	}

	if (strstr(line, "LD") == line) {
		sscanf(line, "LD V%hX, 0x%hX\n", &x, &val);
		ins = 0x6000 | (x << 8 & 0x0F00) | (val & 0x00FF);
		ins = TO_BIG_ENDIAN(ins);
		fwrite(&ins, 2, 1, out);
	} else if (strstr(line, "DRW") == line) {
		/* TODO */
	}
	/* TODO */
}
