#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "chip8.h"

#define OUTFILE_DEFAULTNAME "a.rom"

static void skip_comment(FILE *fp);

int main(int argc, char *argv[])
{
	char *file_name;
	FILE *in_fp;
	int ch;
	unsigned char buf[2];
	int i;
	FILE *out_fp;
	unsigned char high;
	unsigned char low;
	unsigned char b;

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

	i = 0;
	while ((ch = fgetc(in_fp)) != EOF) {
		if (!isalnum(ch)) {
			if (ch == '#') {
				skip_comment(in_fp);
			}
			/* Ignore non-alphanumeric */
			continue;
		}
		if (isalpha(ch)) {
			if (ch < 'A' || ch > 'Z') {
				/* Skip */
				continue;
			}
			ch = ch - 'A' + 0xA;
		} else {
			ch = ch - '0';
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

	fclose(out_fp);
	fclose(in_fp);

	return 0;
}

static void skip_comment(FILE *fp)
{
	int ch;
	while ((ch = fgetc(fp)) != '\n') {
		/* Read until end of line */
	}
}
