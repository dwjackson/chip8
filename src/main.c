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
#include "SDL.h"
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#define USAGE_FMT "Usage: %s [FILE_NAME]\n"
#define DISPLAY_WPIXELS CHIP8_DISPLAYW
#define DISPLAY_HPIXELS CHIP8_DISPLAYH
#define CHIP8_PIXEL_HEIGHT 10
#define CHIP8_PIXEL_WIDTH 10

#define FREQUENCY 44100
#define VOLUME 127.0
#define SAMPLES 8192

static void *setup_renderer(struct chip8_renderer *c8renderer);
static void teardown_display();
static void clear_screen(void *renderer_p);
static void setup_keyboard(struct chip8_keyboard *keyboard);
static void render_display(struct chip8 *chip);
static void render_black(SDL_Renderer *renderer);
static void render_white(SDL_Renderer *renderer);
static byte waitkey();
static int is_key_down(byte key);
static void *timer_thread_update(void *arg);
static void check_kill(struct chip8 *chip);

int main(int argc, char *argv[])
{
	struct chip8 chip;
	char *file_name;
	struct chip8_keyboard keyboard;
	struct chip8_renderer c8renderer;
	pthread_t timer_thread;
	void *renderer;

	if (argc < 2) {
		printf(USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}
	renderer = setup_renderer(&c8renderer);
	setup_keyboard(&keyboard);
	chip8_init(&chip, &keyboard, &c8renderer, check_kill);
	file_name = argv[1];
	if (chip8_load(&chip, file_name) < 0) {
		teardown_display();
		exit(EXIT_FAILURE);
	}
	clear_screen(renderer);
	pthread_create(&timer_thread, NULL, timer_thread_update, &chip);
	chip8_exec(&chip);
	pthread_join(timer_thread, NULL);
	teardown_display();

	return 0;
}

static void *setup_renderer(struct chip8_renderer *c8renderer)
{
	int disph, dispw;
	int ret;
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;

	dispw = DISPLAY_WPIXELS * CHIP8_PIXEL_WIDTH;
	disph = DISPLAY_HPIXELS * CHIP8_PIXEL_HEIGHT;

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
	c8renderer->data = renderer;
	c8renderer->render_display = render_display;

	return renderer;
}

static void setup_keyboard(struct chip8_keyboard *keyboard)
{
	keyboard->waitkey = waitkey;
	keyboard->is_key_down = is_key_down;
}

static void clear_screen(void *renderer_p)
{
	SDL_Renderer *renderer = (SDL_Renderer *) renderer_p;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
}

static void render_display(struct chip8 *chip)
{
	SDL_Rect pixel;
	SDL_Renderer *renderer = chip->renderer->data;
	int i, j;
	byte bit;

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	for (i = 0; i < CHIP8_DISPLAYH; i++) {
		for (j = 0; j < CHIP8_DISPLAYW; j++) {
			bit = chip8_getpixel(chip, j, i);
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
			keycode = 0xFF;
			break;
		}
		break;
	}

	return keycode;
}

struct audiodata {
	float sin_pos;
	float sin_step;
};

static void populate_audio(void *data, Uint8 *stream, int len)
{
	struct audiodata *audiodata = (struct audiodata *)data;
	int i;
	for (i = 0; i < len; i++) {
		stream[i] = (Uint8) (VOLUME * sinf(audiodata->sin_pos)) + 127;
		audiodata->sin_pos += audiodata->sin_step;
	}
}

static void *timer_thread_update(void *arg)
{
	long freq = 770;
	struct chip8 *chip = arg;
	SDL_AudioSpec spec;
	struct audiodata audiodata;
	SDL_AudioStatus status;

	spec.freq = FREQUENCY;
	spec.format = AUDIO_U8;
	spec.channels = 1;
	spec.samples = SAMPLES;
	spec.callback = populate_audio;
	spec.userdata = &audiodata;

	audiodata.sin_pos = 0;
	audiodata.sin_step = 2 * M_PI * freq / FREQUENCY;

	if (SDL_OpenAudio(&spec, NULL) < 0) {
		fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	while (!chip->is_halted) {
		usleep(1000000 / 60); /* 60 Hz */
		if (chip->reg_dt > 0) {
			chip->reg_dt--;
		}
		if (chip->reg_st > 0) {
			chip->reg_st--;
		}
		status = SDL_GetAudioStatus();
		if (chip->reg_st > 0
			&& (status == SDL_AUDIO_STOPPED
				|| status == SDL_AUDIO_PAUSED)) {
			SDL_PauseAudio(0);
		} else if (chip->reg_st == 0 && status == SDL_AUDIO_PLAYING) {
			SDL_PauseAudio(1);
		}
	}
	return NULL;
}

static int is_key_down(byte key)
{
	const Uint8 *state;
	SDL_Scancode code;

	SDL_PumpEvents();
	state = SDL_GetKeyboardState(NULL);
       
	switch (key) {
	case 0x0:
		code = SDL_SCANCODE_M;
		break;
	case 0x1:
		code = SDL_SCANCODE_7;
		break;
	case 0x2:
		code = SDL_SCANCODE_8;
		break;
	case 0x3:
		code = SDL_SCANCODE_9;
		break;
	case 0x4:
		code = SDL_SCANCODE_U;
		break;
	case 0x5:
		code = SDL_SCANCODE_I;
		break;
	case 0x6:
		code = SDL_SCANCODE_O;
		break;
	case 0x7:
		code = SDL_SCANCODE_J;
		break;
	case 0x8:
		code = SDL_SCANCODE_K;
		break;
	case 0x9:
		code = SDL_SCANCODE_L;
		break;
	case 0xA:
		code = SDL_SCANCODE_N;
		break;
	case 0xB:
		code = SDL_SCANCODE_COMMA;
		break;
	case 0xC:
		code = SDL_SCANCODE_0;
		break;
	case 0xD:
		code = SDL_SCANCODE_P;
		break;
	case 0xE:
		code = SDL_SCANCODE_SEMICOLON;
		break;
	case 0xF:
		code = SDL_SCANCODE_PERIOD;
		break;
	default:
		return 1;
	}
	return state[code];
}

static void check_kill(struct chip8 *chip)
{
	SDL_Event event;
	SDL_PollEvent(&event);
	if (event.type == SDL_QUIT) {
		chip8_halt(chip);
	}
}

static void teardown_display()
{
	SDL_Quit();
}
