#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "chip8.h"

void chip8_ret(struct chip8 *chip);
void chip8_call(struct chip8 *chip, unsigned short ins);
void chip8_jump(struct chip8 *chip, unsigned short ins);
void chip8_sne(struct chip8 *chip, unsigned short ins);
void chip8_load_i(struct chip8 *chip, unsigned short ins);
void chip8_load_immediate(struct chip8 *chip, unsigned short ins);

#endif /* INSTRUCTIONS_H */
