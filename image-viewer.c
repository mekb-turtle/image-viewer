#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <SDL2/SDL.h>
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr strerror(errno)
#define BLOCK 1024
struct image {
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	uint32_t width;
	uint32_t height;
	uint32_t *pixels;
	FILE *fp;
} image;
uint32_t read_size(FILE *fp) {
	int i4, i3, i2, i1;
	if ((i4 = fgetc(fp)) < 0 || (i3 = fgetc(fp)) < 0 || (i2 = fgetc(fp)) < 0 || (i1 = fgetc(fp)) < 0)
		return 0;
	return (i4 << 030) | (i3 << 020) | (i2 << 010) | i1;
}
bool parse_image(struct image *img) {
	for (char i = 0; i < 8; ++i) {
		if (fgetc(img->fp) != "farbfeld"[i]) {
			return 0;
		}
	}
	if ((img->width  = read_size(img->fp)) <= 0) return 0;
	if ((img->height = read_size(img->fp)) <= 0) return 0;
	img->pixels = NULL;
	size_t len = 0;
	size_t real_len = 0;
	for (uint32_t y = 0; y < img->height; ++y)
	for (uint32_t x = 0; x < img->width;  ++x) {
		int a, r, g, b;
		r = fgetc(img->fp); fgetc(img->fp);
		g = fgetc(img->fp); fgetc(img->fp);
		b = fgetc(img->fp); fgetc(img->fp);
		a = fgetc(img->fp); fgetc(img->fp);
		if (a < 0) {
			free(img->pixels);
			return 0;
		}
		++len;
		if (len > real_len) {
			real_len += BLOCK;
			img->pixels = realloc(img->pixels, real_len * (sizeof(uint32_t)/sizeof(uint8_t)));
			if (!img->pixels) {
				eprintf("realloc: %s\n", strerr); return 0;
			}
		}
		img->pixels[len-1] = (a << 24) | (r << 16) | (g << 8) | b;
	}
	if (ferror(img->fp)) { free(img->pixels); return 0; }
	return 1;
}
bool create_render(struct image *img, SDL_Window *win) {
	img->renderer = SDL_CreateRenderer(win, -1, 0);
	if (!img->renderer) return 0;
	img->texture = SDL_CreateTexture(img->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, img->width, img->height);
	if (!img->texture) return 0;
	SDL_UpdateTexture(img->texture, NULL, img->pixels, img->width*4);
	SDL_RenderClear(img->renderer);
	SDL_RenderCopy(img->renderer, img->texture, NULL, NULL);
	return 1;
}
void display(struct image *img) {
	SDL_RenderPresent(img->renderer);
}
void dispose(struct image *img) {
	SDL_DestroyTexture(img->texture);
	SDL_DestroyRenderer(img->renderer);
	free(img->pixels);
}
bool quitted = 0;
void quit() {
	if (quitted) return;
	quitted = 1;
	dispose(&image);
	SDL_Quit();
}
int usage(char *argv0) {
	eprintf("\
Usage: %s [filename]\n", argv0);
	return 2;
}
int main(int argc, char* argv[]) {
#define INVALID return usage(argv[0])
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		eprintf("error initialising SDL: %s\n", SDL_GetError()); return 1;
	}
	bool flag_done = 0;
	char *filename = NULL;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; else // -- denotes end of flags
			INVALID;
		} else {
			if (filename) INVALID;
			filename = argv[i];
		}
	}
	if (!filename) INVALID;
	FILE *fp;
	if (filename[0] == '-' && filename[1] == '\0') {
		fp = stdin;
		filename = "stdin";
	} else {
		fp = fopen(filename, "r");
		if (!fp) { eprintf("fopen: %s: %s\n", filename, strerr); return errno; }
	}
	image.fp = fp;
	if (parse_image(&image) == 0) {
		eprintf("Failed reading farbfeld image\n"); return 1;
	}
	char *title = malloc(16 + strlen(filename));
	if (!title) {
		eprintf("malloc: %s\n", strerr); return 0;
	}
	sprintf(title, "Image: %s", filename);
	SDL_Window *win = SDL_CreateWindow(title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		image.width, image.height, 0);
	if (create_render(&image, win) == 0) {
		eprintf("Failed creating render\n");
		quit();
		return 1;
	}
	bool quit_ = 0;
	atexit(quit);
	display(&image);
	SDL_Event event;
	while (!quit_) {
		if (!SDL_PollEvent(&event)) continue;
		switch (event.type) {
			case SDL_QUIT:
				quit_ = 1;
				break;
		}
	}
	quit();
	return 0;
}
