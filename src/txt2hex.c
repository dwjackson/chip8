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
#include <ctype.h>
#include "chip8.h"

#define OUTFILE_DEFAULTNAME "a.rom"
#define LINE_COMMENT_START '#'

static void skip_comment(FILE *fp);
static void translate(FILE *in_fp, FILE *out_fp);

int main(int argc, char *argv[])
{
	char *file_name;
	FILE *in_fp;
	FILE *out_fp;

	if (argc < 2) {
		in_fp = stdin;
	} else {
		file_name = argv[1];
		in_fp = fopen(file_name, "r");
		if (!in_fp) {
			perror(file_name);
			exit(EXIT_FAILURE);
		}
	}

	out_fp = fopen(OUTFILE_DEFAULTNAME, "wb");
	if (!out_fp) {
		perror(OUTFILE_DEFAULTNAME);
		exit(EXIT_FAILURE);
	}

	translate(in_fp, out_fp);	

	fclose(out_fp);
	fclose(in_fp);

	return 0;
}

static void translate(FILE *in_fp, FILE *out_fp)
{
	int ch;
	int i;
	unsigned char buf[2];
	unsigned char high;
	unsigned char low;
	unsigned char b;

	i = 0;
	while ((ch = fgetc(in_fp)) != EOF) {
		if (!isalnum(ch) && ch == LINE_COMMENT_START) {
			skip_comment(in_fp);
			continue;
		}
		if (!isalnum(ch)) {
			/* Ignore non-alphanumeric */
			continue;
		}

		if (isalpha(ch) && islower(ch)) {
			ch = ch - 'a' + 0xA;
		} else if (isalpha(ch) && isupper(ch)) {
			ch = ch - 'A' + 0xA;
		} else if (isdigit(ch)) {
			ch = ch - '0';
		} else {
			/* Weird character, skip it */
			continue;
		}

		buf[i++] = ch;
		if (i > 1) {
			high = (buf[0] << 4) & 0xF0;
			low = buf[1] & 0x0F;
			b = high | low;
			fwrite(&b, 1, 1, out_fp);
			i = 0;
		}
	}
}

static void skip_comment(FILE *fp)
{
	int ch;
	while ((ch = fgetc(fp)) != '\n') {
		/* Read until end of line */
	}
}

