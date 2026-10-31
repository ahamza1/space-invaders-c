#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "platform.h"
#include "constants.h"
#include "font_8x8.h"

#define TARGET_FPS 60

static SDL_Window   *sdl_window;
static SDL_Renderer *sdl_renderer;
static SDL_Texture  *sdl_texture;
static SDL_Rect      visible_rect;
static Uint64        last_frame_counter;

static window_t  main_window;
static context_t main_context;

// Also shown as a message box, since Windows builds have no console to print to
static void fatal(const char *format, ...) {
	char message[1024];
	va_list args;

	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	fprintf(stderr, "%s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Space Invaders", message, sdl_window);
	exit(1);
}

window_t *window_create(int width, int height, int hidden_top_rows, const char *title) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fatal("SDL_Init failed: %s", SDL_GetError());
	}

	// The game draws in klika_os window coordinates, where the top rows were the OS titlebar.
	// They are kept in the framebuffer but cropped out when presenting.
	visible_rect = (SDL_Rect) { 0, hidden_top_rows, width, height - hidden_top_rows };

	sdl_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		visible_rect.w, visible_rect.h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
	sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	if (!sdl_window || !sdl_renderer || !sdl_texture) {
		fatal("SDL setup failed: %s", SDL_GetError());
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_RenderSetLogicalSize(sdl_renderer, visible_rect.w, visible_rect.h);
	SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_NONE);

	main_context.buffer = calloc((size_t)width * height, sizeof(uint32_t));
	main_context.width = width;
	main_context.height = height;

	main_window.context = &main_context;
	main_window.should_close = 0;

	last_frame_counter = SDL_GetPerformanceCounter();

	return &main_window;
}

void window_close(window_t *window) {
	free(window->context->buffer);
	SDL_DestroyTexture(sdl_texture);
	SDL_DestroyRenderer(sdl_renderer);
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();
}

static int translate_key(SDL_Keycode key) {
	switch (key) {
		case SDLK_LEFT:     return KEY_LEFT;
		case SDLK_RIGHT:    return KEY_RIGHT;
		case SDLK_UP:       return KEY_UP;
		case SDLK_DOWN:     return KEY_DOWN;
		case SDLK_SPACE:    return KEY_SPACE;
		case SDLK_RETURN:
		case SDLK_KP_ENTER: return KEY_RETURN;
		default:            return 0;
	}
}

// Returns the next key press/release in msg, or msg->message == 0 when there is none.
// Other events are drained so they don't delay input.
void window_peek_message(window_t *window, message_t *msg) {
	SDL_Event event;

	msg->message = 0;
	msg->key = 0;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			window->should_close = 1;
		} else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				window->should_close = 1;
				continue;
			}

			int key = translate_key(event.key.keysym.sym);
			if (key == 0 || event.key.repeat) {
				continue;
			}

			msg->message = event.type == SDL_KEYDOWN ? MESSAGE_KEY_PRESS : MESSAGE_KEY_RELEASE;
			msg->key = key;
			return;
		}
	}
}

static void wait_for_next_frame(void) {
	Uint64 frame_ticks = SDL_GetPerformanceFrequency() / TARGET_FPS;
	Uint64 elapsed = SDL_GetPerformanceCounter() - last_frame_counter;

	if (elapsed < frame_ticks) {
		Uint32 remaining_ms = (Uint32)((frame_ticks - elapsed) * 1000 / SDL_GetPerformanceFrequency());
		if (remaining_ms > 1) {
			SDL_Delay(remaining_ms - 1);
		}
		while (SDL_GetPerformanceCounter() - last_frame_counter < frame_ticks) {
		}
	}

	last_frame_counter = SDL_GetPerformanceCounter();
}

void window_present(window_t *window) {
	context_t *context = window->context;

	SDL_UpdateTexture(sdl_texture, NULL, context->buffer, context->width * (int)sizeof(uint32_t));
	SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
	SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, sdl_texture, &visible_rect, NULL);
	SDL_RenderPresent(sdl_renderer);

	wait_for_next_frame();
}

int rand_range(int lower, int upper) {
	return (rand() % (upper - lower + 1)) + lower;
}

static int clamp(int value, int min, int max) {
	return value < min ? min : (value > max ? max : value);
}

void gfx_hline(context_t *context, int x1, int x2, int y, uint32_t color) {
	if (y < 0 || y >= context->height) {
		return;
	}

	x1 = clamp(x1, 0, context->width - 1);
	x2 = clamp(x2, 0, context->width - 1);

	for (int x = x1; x <= x2; x++) {
		CONTEXT_32[FIRST_PIXEL(x, y)] = color;
	}
}

void gfx_fillrect(context_t *context, int x1, int y1, int x2, int y2, uint32_t color) {
	for (int y = y1; y <= y2; y++) {
		gfx_hline(context, x1, x2, y, color);
	}
}

static void gfx_putchar(context_t *context, int x, int y, uint32_t fgcolor, uint32_t bgcolor, char c) {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			if (x + i >= 0 && x + i < context->width && y + j >= 0 && y + j < context->height) {
				CONTEXT_32[FIRST_PIXEL(x + i, y + j)] = ((font8x8_basic[c & 0x7F][j] >> i) & 1) ? fgcolor : bgcolor;
			}
		}
	}
}

void gfx_puts(context_t *context, int x, int y, uint32_t fgcolor, uint32_t bgcolor, const char *text) {
	while (*text) {
		gfx_putchar(context, x, y, fgcolor, bgcolor, *text++);
		x += 8;
	}
}

static FILE *open_asset(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (file != NULL) {
		return file;
	}

	// Fall back to the executable's directory so the game can be started from anywhere.
	char *base_path = SDL_GetBasePath();
	if (base_path != NULL) {
		char path[1024];
		snprintf(path, sizeof(path), "%s%s", base_path, filename);
		SDL_free(base_path);
		file = fopen(path, "rb");
	}

	return file;
}

// Loads the raw BMP file into memory, same as klika_os did. The game relies on the in-memory
// layout (32bpp, bottom-up rows) to edit shield pixels directly.
void bmp_from_file(const char *filename, bmp_image_t *bmp_out) {
	FILE *file = open_asset(filename);
	if (file == NULL) {
		fatal("Could not open asset '%s'", filename);
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Extra padding: shield collision code may read one pixel past the end of the image data
	uint8_t *buffer = calloc(1, (size_t)size + 512);
	if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
		fatal("Could not read asset '%s'", filename);
	}
	fclose(file);

	bmp_out->buffer = buffer;
	bmp_out->header = (bmp_header_t*)buffer;
	bmp_out->data = (uint32_t*)(buffer + bmp_out->header->offset);

	if (bmp_out->header->bits_per_pixel != 32) {
		fatal("Asset '%s' must be a 32-bit BMP", filename);
	}
}

void bmp_close(bmp_image_t *bmp) {
	free(bmp->buffer);
	bmp->buffer = NULL;
	bmp->header = NULL;
	bmp->data = NULL;
}

static void put_clipped_pixel(context_t *context, int x, int y, uint32_t color) {
	if (x >= 0 && x < context->width && y >= 0 && y < context->height) {
		CONTEXT_32[FIRST_PIXEL(x, y)] = color;
	}
}

void bmp_blit_clipped(context_t *context, bmp_image_t *bmp, int x, int y, int clip_x, int clip_y, int clip_w, int clip_h) {
	int bmp_width = bmp->header->width_px;
	int bmp_height = bmp->header->height_px;

	// If negative : stored TOP -> BOTTOM
	if (bmp_height < 0) {
		uint32_t *src = bmp->data + (clip_y * bmp_width + clip_x);
		for (int i = 0; i < clip_h; i++) {
			for (int j = 0; j < clip_w; j++) {
				put_clipped_pixel(context, x + j, y + i, *src++);
			}
			src += (bmp_width - clip_w);
		}
	} else {
		// If positive : stored BOTTOM -> TOP ... make inverted blit (height - i)
		uint32_t *src = bmp->data + ((bmp_height - clip_y - clip_h) * bmp_width + clip_x);
		for (int i = 0; i < clip_h; i++) {
			for (int j = 0; j < clip_w; j++) {
				put_clipped_pixel(context, x + j, y + (clip_h - 1 - i), *src++);
			}
			src += (bmp_width - clip_w);
		}
	}
}
