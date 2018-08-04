# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2018 David Jackson

CFLAGS = -g -O0 -Wall -Wextra

bin_PROGRAMS = chip8 dis8 txt2hex dump8 asm8

chip8_SOURCES = main.c chip8.c chip8.h instructions.c instructions.h
chip8_LDADD = -lSDL2 -lpthread -lm

dis8_SOURCES = dis8.c

txt2hex_SOURCES = txt2hex.c chip8.h

dump8_SOURCES = dump8.c

asm8_SOURCES = asm8.c asm8.h fsm.c fsm.h encode.h encode.c
