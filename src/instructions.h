#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "chip8.h"

#define BIT(b, i) (((b) & (0x1 << (i))) >> (i))

void chip8_ret(struct chip8 *chip);
void chip8_call(struct chip8 *chip, unsigned short ins);
void chip8_jump(struct chip8 *chip, unsigned short ins);
void chip8_se_immediate(struct chip8 *chip, unsigned short ins);
void chip8_se(struct chip8 *chip, unsigned short ins);
void chip8_sne(struct chip8 *chip, unsigned short ins);
void chip8_load_i(struct chip8 *chip, unsigned short ins);
void chip8_load_immediate(struct chip8 *chip, unsigned short ins);
void chip8_draw(struct chip8 *chip, unsigned short ins);
void chip8_add_immediate(struct chip8 *chip, unsigned short ins);
void chip8_add(struct chip8 *chip, unsigned short ins);
void chip8_sub(struct chip8 *chip, unsigned short ins);
void chip8_cls(struct chip8 *chip);
void chip8_ld(struct chip8 *chip, unsigned short ins);
void chip8_waitkey(struct chip8 *chip, unsigned short ins);
void chip8_load_from_dt(struct chip8* chip, unsigned short ins);
void chip8_load_dt(struct chip8 *chip, unsigned short ins);
void chip8_load_st(struct chip8 *chip, unsigned short ins);
void chip8_load_range_from_i(struct chip8 *chip, unsigned short ins);
void chip8_store_bcd(struct chip8 *chip, unsigned short ins);
void chip8_load_i_hexfont(struct chip8 *chip, unsigned short ins);
void chip8_rnd(struct chip8 *chip, unsigned short ins);
void chip8_or(struct chip8 *chip, unsigned short ins);
void chip8_and(struct chip8 *chip, unsigned short ins);
void chip8_skp(struct chip8 *chip, unsigned short ins);
void chip8_sknp(struct chip8 *chip, unsigned short ins);
void chip8_shr(struct chip8 *chip, unsigned short ins);
void chip8_subn(struct chip8 *chip, unsigned short ins);
void chip8_shl(struct chip8 *chip, unsigned short ins);

#endif /* INSTRUCTIONS_H */
