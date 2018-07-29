#include "instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "chip8.h"

static int chip8_pushpc(struct chip8 *chip)
{
	if (chip->sp + 1 > CHIP8_STACKSIZE) {
		return 1;
	}
	chip->sp++;
	chip->stack[chip->sp] = chip->pc;
	return 0;
}

static int chip8_poppc(struct chip8 *chip)
{
	if (chip->sp - 1 < 0) {
		return 1;
	}
	chip->pc = chip->stack[chip->sp];
	chip->sp--;
	return 0;
}

void chip8_ret(struct chip8 *chip)
{
	chip8_poppc(chip);
}

void chip8_call(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr;

	/* CALL addr */
	addr = ins & 0x0FFF;
	chip8_pushpc(chip);
	chip->pc = addr;
}

void chip8_jump(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr;

	/* JP addr */
	addr = ins & 0x0FFF;
	if (addr > CHIP8_RAMBYTES) {
		fprintf(stderr, "Invalid jump\n");
		abort();
	}
	chip->pc = addr;
}

/* 3xkk - skip next instruction if Vx = kk */
void chip8_sne(struct chip8 *chip, unsigned short ins)
{
	byte x, y;
	byte regval;

	/* SE Vx, byte */
	x = (ins & 0x0F00) >> 8;
	y = ins & 0x00FF;
	regval = chip->reg_v[x];
	if (regval == y) {
		chip->pc += 2;
	}
}

void chip8_load_i(struct chip8 *chip, unsigned short ins)
{
	unsigned short addr;
	/* LD I, addr */
	addr = ins & 0x0FFF;
	chip->reg_i = addr;
}

void chip8_load_immediate(struct chip8 *chip, unsigned short ins)
{
	byte x, y;

	/* LD Vx, byte */
	x = (ins & 0x0F00) >> 8;
	y = ins & 0x00FF;
	chip->reg_v[x] = y;
}

static void render_black(SDL_Renderer *renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

static void render_white(SDL_Renderer *renderer)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
}

static void render_display(struct chip8 *chip, SDL_Renderer *renderer)
{
	SDL_Rect pixel;
	int i, j;
	byte bit;

	for (i = 0; i < CHIP8_DISPLAYH; i++) {
		for (j = 0; j < CHIP8_DISPLAYW; j++) {
			bit = chip->display[i][j];
			pixel.x = j * CHIP8_PIXEL_WIDTH;
			pixel.y = i * CHIP8_PIXEL_HEIGHT;
			pixel.w = CHIP8_PIXEL_WIDTH;
			pixel.h = CHIP8_PIXEL_HEIGHT;

			if (bit == 0) {
				render_black(renderer);
			} else {
				render_white(renderer);
			}

			if (SDL_RenderFillRect(renderer, &pixel) != 0) {
				fprintf(stderr,
					"SDL_RenderFillRect failed: %s\n",
					SDL_GetError());
			}
		}
	}
	SDL_RenderPresent(renderer);
}

static int is_within_display(int x, int y)
{
	return x >= 0 && y >= 0 && x <= CHIP8_DISPLAYW && y <= CHIP8_DISPLAYH;
}

static void print_sprite(byte *sprite, int n)
{
	int i, j;
	int bit;
	for (i = 0; i < n; i++) {
		for (j = 7; j >= 0; j--) {
			bit = BIT(sprite[i], j);
			if (bit == 1) {
				printf("*");
			} else {
				printf(" ");
			}
		}
		printf("\n");
	}
}

/* DRW Vx, Vy, byte */
void chip8_draw(struct chip8 *chip, unsigned short ins, SDL_Renderer *renderer)
{
	byte x, y;
	byte n; /* Sprite length */
	unsigned short addr;
	byte vx, vy;
	byte sprite[CHIP8_SPRITEBYTES];
	int i, j;
	int bit;
	int display_x, display_y;

	x = (ins & 0x0F00) >> 8;
	y = (ins & 0x00F0) >> 4;
	n = ins & 0x000F;
	if (n > CHIP8_SPRITEBYTES) {
		fprintf(stderr, "Invalid sprite length: 0x%04X\n", ins);
		exit(EXIT_FAILURE);
	}

	addr = chip->reg_i;
	if (addr < CHIP8_PROGSTART || addr >= CHIP8_RAMBYTES) {
		fprintf(stderr, "Invalid draw address: 0x%04X\n", addr);
		exit(EXIT_FAILURE);
	}

	vx = chip->reg_v[x];
	vy = chip->reg_v[y];

	for (i = 0; i < n; i++) {
		sprite[i] = chip->ram[addr + i];
	}
	/* print_sprite(sprite, n); */ /* DEBUG */

	for (i = 0; i < n; i++) {
		for (j = 7; j >= 0; j--) {
			display_x = vx + (7 - j);
			display_y = vy + i;
			if (is_within_display(display_x, display_y)) {
				bit = BIT(sprite[i], j);
				chip->display[display_y][display_x] = bit;
			}
		}
	}
	render_display(chip, renderer);
}

void chip8_add_immediate(struct chip8 *chip, unsigned short ins)
{
	byte x, y;

	/* ADD Vx, byte */
	x = (ins & 0x0F00) >> 8;
	y = ins & 0x00FF;
	chip->reg_v[x] += y;
}

void chip8_add(struct chip8 *chip, unsigned short ins)
{
	byte x, y;

	x = (ins & 0x0F00) >> 8;
	y = (ins & 0x00F0) >> 4;

	/* ADD Vx, Vy */
	chip->reg_v[x] = chip->reg_v[x] + chip->reg_v[y];
}

void chip8_cls(struct chip8 *chip, SDL_Renderer *renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
}

void chip8_ld(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte y = (ins & 0x00F0) >> 4;
	chip->reg_v[x] = chip->reg_v[y];
}

void chip8_waitkey(struct chip8 *chip, unsigned short ins)
{
	byte x;
	SDL_Event event;
	byte keycode;

	/* LD Vx, K */
	x = (ins & 0x0F00) >> 8;
	while (1) {
		SDL_WaitEvent(&event);
		if (event.type != SDL_KEYDOWN) {
			continue;
		}
		switch (event.key.keysym.sym) {
		case SDLK_7:
			keycode = 0x1;
			break;
		case SDLK_8:
			keycode = 0x2;
			break;
		case SDLK_9:
			keycode = 0x3;
			break;
		case SDLK_0:
			keycode = 0xC;
			break;
		case SDLK_u:
			keycode = 0x4;
			break;
		case SDLK_i:
			keycode = 0x5;
			break;
		case SDLK_o:
			keycode = 0x6;
			break;
		case SDLK_p:
			keycode = 0xD;
			break;
		case SDLK_j:
			keycode = 0x7;
			break;
		case SDLK_k:
			keycode = 0x8;
			break;
		case SDLK_l:
			keycode = 0x9;
			break;
		case SDLK_SEMICOLON:
			keycode = 0xE;
			break;
		case SDLK_n:
			keycode = 0xA;
			break;
		case SDLK_m:
			keycode = 0x0;
			break;
		case SDLK_COMMA:
			keycode = 0xB;
			break;
		case SDLK_PERIOD:
			keycode = 0xF;
			break;
		default:
			break;
		}
		break;
	}

	chip->reg_v[x] = keycode;
}

void chip8_load_from_dt(struct chip8* chip, unsigned short ins)
{
	/* LD Vx, DT */
	byte x = (ins & 0x0F00) >> 8;
	chip->reg_v[x] = chip->dt;
}

void chip8_load_dt(struct chip8 *chip, unsigned short ins)
{
	/* LD DT, Vx */
	byte x = (ins & 0x0F00) >> 8;
	chip->dt = chip->reg_v[x];
}

void chip8_load_st(struct chip8 *chip, unsigned short ins)
{
	/* LD ST, Vx */
	byte x = (ins & 0x0F00) >> 8;
	chip->st = chip->reg_v[x];
}

void chip8_load_range_from_i(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	int i;

	/* LD Vx, [I] */
	for (i = 0; i < x; i++) {
		chip->reg_v[i] = chip->ram[chip->reg_i + i];
	}
}

void chip8_store_bcd(struct chip8 *chip, unsigned short ins)
{
	byte x = (ins & 0x0F00) >> 8;
	byte val = chip->reg_v[x];
	byte i = chip->reg_i;
	byte hundreds = val / 100;
	byte tens = (val - hundreds * 100) / 10;
	byte ones = val - hundreds * 100 - tens * 10;

	if (i + 2 >= CHIP8_RAMBYTES) {
		fprintf(stderr, "RAM overflow\n");
		abort();
	}

	chip->ram[i] = hundreds;
	chip->ram[i + 1] = tens;
	chip->ram[i + 2] = ones;
}
