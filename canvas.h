#pragma once

struct canvas_ctx;
#define LIBCANVAS_MALLOC(x) malloc(x)
#define LIBCANVAS_CALLOC(nmemb, sz) calloc(nmemb, sz)
#define LIBCANVAS_REALLOC(old, newsz) realloc(old, newsz)
#define LIBCANVAS_FREE(x) free(x)
#define LIBCANVAS_DEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define LIBCANVAS_PREFIX(x) canvas_##x

struct canvas_color {
  unsigned char r, g, b, a;
} __attribute__((packed));

static inline struct canvas_color canvas_color_rgba(unsigned char r,
                                                    unsigned char g,
                                                    unsigned char b,
                                                    unsigned char a) {
  return (struct canvas_color){
    .r = r,
    .g = g,
    .b = b,
    .a = a,
  };
}

static inline struct canvas_color canvas_color_rgb(unsigned char r,
                                                  unsigned char g,
                                                  unsigned char b) {
  return (struct canvas_color){
    .r = r,
    .g = g,
    .b = b,
    .a = 0xff,
  };
}

typedef enum {
  // Each pixel is a 32-bit quantity with the
  // alpha, red, green, blue channels stored in the lower bits
  LIBCANVAS_FORMAT_ARGB32,
  // Each pixel is a 32-bit quantity with the
  // red, green, blue channels stored in the lower bits
  LIBCANVAS_FORMAT_RGB24,
} canvas_format;

struct canvas_ctx *LIBCANVAS_PREFIX(ctx_create)(int width, int height, canvas_format format);
void LIBCANVAS_PREFIX(ctx_destroy)(struct canvas_ctx *);
unsigned char *LIBCANVAS_PREFIX(ctx_get_surface)(struct canvas_ctx *ctx);
canvas_format LIBCANVAS_PREFIX(ctx_get_format)(struct canvas_ctx *ctx);
int LIBCANVAS_PREFIX(ctx_get_width)(struct canvas_ctx *ctx);
int LIBCANVAS_PREFIX(ctx_get_height)(struct canvas_ctx *ctx);

int LIBCANVAS_PREFIX(ctx_resize_buffer)(struct canvas_ctx *ctx, int width, int height);

// Rectangles
void LIBCANVAS_PREFIX(ctx_fill_rect)(struct canvas_ctx *ctx,
                                     int x, int y, int width, int height,
                                     struct canvas_color color);
void LIBCANVAS_PREFIX(ctx_stroke_rect)(struct canvas_ctx *ctx,
                                     int x, int y, int width, int height,
                                     struct canvas_color color);

// Lines
void LIBCANVAS_PREFIX(ctx_stroke_line)(struct canvas_ctx *ctx,
                                       int x0, int y0, int x1, int y1,
                                       struct canvas_color color);

// Circles
void LIBCANVAS_PREFIX(ctx_fill_circle)(struct canvas_ctx *ctx,
                                       int x, int y, int rad,
                                       struct canvas_color color);
void LIBCANVAS_PREFIX(ctx_stroke_circle)(struct canvas_ctx *ctx,
                                         int x, int y, int rad,
                                         struct canvas_color color);

// implementation
#ifdef LIBCANVAS_IMPLEMENTATION

#endif
