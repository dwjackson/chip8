#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "chip8.h"

void chip8_ret(struct chip8 *chip);
void chip8_call(struct chip8 *chip, unsigned short ins);
void chip8_jump(struct chip8 *chip, unsigned short ins);
void chip8_sne(struct chip8 *chip, unsigned int ins);

#endif /* INSTRUCTIONS_H */
