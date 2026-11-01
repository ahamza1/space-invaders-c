#ifndef __PLATFORM_H__
#define __PLATFORM_H__

// SDL2-backed platform, drawing goes into a 32-bit ARGB software framebuffer that is uploaded to SDL on window_present().

#include <stdint.h>

#define MESSAGE_KEY_PRESS        4
#define MESSAGE_KEY_RELEASE      5

#define CONTEXT_32 ((uint32_t*)context->buffer)
#define FIRST_PIXEL(x, y) ((x) + ((y) * (context->width)))

typedef struct {
	uint32_t *buffer;
	int width;
	int height;
} context_t;

typedef struct {
	context_t *context;
	int should_close;
} window_t;

typedef struct {
	int message;
	int key;
} message_t;

// Packed to match the on-disk BMP header layout (#pragma pack works on GCC, Clang and MSVC)
#pragma pack(push, 1)
typedef struct {
	uint16_t type;              // Magic identifier: 0x4d42
	uint32_t size;              // File size in bytes
	uint16_t reserved1;         // Not used
	uint16_t reserved2;         // Not used
	uint32_t offset;            // Offset to image data in bytes from beginning of file
	uint32_t dib_header_size;   // DIB Header size in bytes
	int32_t  width_px;          // Width of the image
	int32_t  height_px;         // Height of image
	uint16_t num_planes;        // Number of color planes
	uint16_t bits_per_pixel;    // Bits per pixel
	uint32_t compression;       // Compression type
	uint32_t image_size_bytes;  // Image size in bytes
	int32_t  x_resolution_ppm;  // Pixels per meter
	int32_t  y_resolution_ppm;  // Pixels per meter
	uint32_t num_colors;        // Number of colors
	uint32_t important_colors;  // Important colors
} bmp_header_t;
#pragma pack(pop)

typedef struct {
	uint8_t *buffer;
	bmp_header_t *header;
	uint32_t *data;
} bmp_image_t;

window_t *window_create(int width, int height, int hidden_top_rows, const char *title);
void window_close(window_t *window);
void window_peek_message(window_t *window, message_t *msg);
void window_present(window_t *window);

void gfx_hline(context_t *context, int x1, int x2, int y, uint32_t color);
void gfx_fillrect(context_t *context, int x1, int y1, int x2, int y2, uint32_t color);
void gfx_puts(context_t *context, int x, int y, uint32_t fgcolor, uint32_t bgcolor, const char *text);

void bmp_from_file(const char *filename, bmp_image_t *bmp_out);
void bmp_close(bmp_image_t *bmp);
void bmp_blit_clipped(context_t *context, bmp_image_t *bmp, int x, int y, int clip_x, int clip_y, int clip_w, int clip_h);

// Random integer in [lower, upper], inclusive
int rand_range(int lower, int upper);

#endif
