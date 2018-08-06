/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#include <stdio.h>

typedef unsigned char byte;
typedef unsigned short instruction;

void disassemble(FILE *fp);
void disassemble_instruction(instruction ins, unsigned short ins_addr);

#endif /* DISASSEMBLE_H */
