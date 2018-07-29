#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "chip8.h"
#include "SDL.h"

#define CHIP8_PIXEL_HEIGHT 10
#define CHIP8_PIXEL_WIDTH 10

#define BIT(b, i) (((b) & (0x1 << (i))) >> (i))

void chip8_ret(struct chip8 *chip);
void chip8_call(struct chip8 *chip, unsigned short ins);
void chip8_jump(struct chip8 *chip, unsigned short ins);
void chip8_sne(struct chip8 *chip, unsigned short ins);
void chip8_load_i(struct chip8 *chip, unsigned short ins);
void chip8_load_immediate(struct chip8 *chip, unsigned short ins);
void chip8_draw(struct chip8 *chip, unsigned short ins, SDL_Renderer *renderer);
void chip8_add_immediate(struct chip8 *chip, unsigned short ins);
void chip8_add(struct chip8 *chip, unsigned short ins);
void chip8_cls(struct chip8 *chip, SDL_Renderer *renderer);
void chip8_ld(struct chip8 *chip, unsigned short ins);
void chip8_waitkey(struct chip8 *chip, unsigned short ins);
void chip8_load_from_dt(struct chip8* chip, unsigned short ins);
void chip8_load_dt(struct chip8 *chip, unsigned short ins);
void chip8_load_st(struct chip8 *chip, unsigned short ins);

#endif /* INSTRUCTIONS_H */
