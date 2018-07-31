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

#define USAGE_FMT "Usage: %s [FILE_NAME]\n"
#define COLS 79

void hexdump(FILE *fp);

int main(int argc, char *argv[])
{
	char *file_name;
	FILE *fp;

	if (argc < 2) {
		printf(USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}
	file_name = argv[1];
	fp = fopen(file_name, "rb");
	if (!fp) {
		perror(file_name);
		exit(EXIT_FAILURE);
	}

	hexdump(fp);

	fclose(fp);

	return 0;
}

void hexdump(FILE *fp)
{
	int ch;
	int i = 0;
	while ((ch = fgetc(fp)) != EOF) {
		if (i + 3 > COLS) {
			i = 0;
			printf("\n");
		}
		printf("%02X ", ch);
		i += 3;
	}
	printf("\n");
}
