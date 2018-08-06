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
#include "chip8.h"
#include "disassemble.h"

int main(int argc, char *argv[])
{
	FILE *fp;
	char *file_name;

	if (argc < 2) {
		fp = stdin;
	} else {
		file_name = argv[1];
		fp = fopen(file_name, "rb");
		if (!fp) {
			perror(file_name);
			exit(EXIT_FAILURE);
		}
	}
	disassemble(fp);
	fclose(fp);
	return 0;
}
