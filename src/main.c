#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"
#include "SDL.h"

#define USAGE_FMT "Usage: %s [FILE_NAME]\n"
#define DISPLAY_WPIXELS 64
#define DISPLAY_HPIXELS 32
#define CHIP8_PIXEL_HEIGHT 10
#define CHIP8_PIXEL_WIDTH 10

static void render_display(struct chip8 *chip);
static void render_black(SDL_Renderer *renderer);
static void render_white(SDL_Renderer *renderer);
byte waitkey();

int main(int argc, char *argv[])
{
	struct chip8 chip;
	char *file_name;
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_bool done = SDL_FALSE;
	SDL_Event event;
	int disph, dispw;
	dispw = DISPLAY_WPIXELS * CHIP8_PIXEL_WIDTH;
	disph = DISPLAY_HPIXELS * CHIP8_PIXEL_HEIGHT;
	int ret;
	struct chip8_renderer c8renderer;

	if (argc < 2) {
		printf(USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		perror("SDL_Init");
		exit(EXIT_FAILURE);
	}
	ret = SDL_CreateWindowAndRenderer(dispw, disph, 0, &window, &renderer);
	if (ret != 0) {
		SDL_Quit();
		perror("SDL_CreateWindowAndRenderer");
		exit(EXIT_FAILURE);
	}

	c8renderer.data = renderer;
	c8renderer.render_display = render_display;
	chip8_init(&chip, &waitkey, &c8renderer);
	file_name = argv[1];
	chip8_load(&chip, file_name);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	chip8_exec(&chip);
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				done = SDL_TRUE;
			}
		}
	}

	SDL_Quit();

	return 0;
}

static void render_display(struct chip8 *chip)
{
	SDL_Rect pixel;
	SDL_Renderer *renderer = chip->renderer->data;
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

static void render_black(SDL_Renderer *renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

static void render_white(SDL_Renderer *renderer)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
}

byte waitkey()
{
	SDL_Event event;
	byte keycode;

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

	return keycode;
}
