#pragma once

// header
struct canvas_ctx;
#define LIBCANVAS_MALLOC(x) malloc(x)
#define LIBCANVAS_CALLOC(nmemb, sz) calloc(nmemb, sz)
#define LIBCANVAS_FREE(x) free(x)
#define LIBCANVAS_DEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define LIBCANVAS_PREFIX(x) canv_##x

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

// Rectangles
void LIBCANVAS_PREFIX(ctx_fill_rect)(struct canvas_ctx *ctx,
                                     int x, int y, int width, int height,
                                     unsigned char r,
                                     unsigned char g,
                                     unsigned char b,
                                     unsigned char a);

// Lines
void LIBCANVAS_PREFIX(ctx_stroke_line)(struct canvas_ctx *ctx,
                                     int x0, int y0, int x1, int y1,
                                     unsigned char r,
                                     unsigned char g,
                                     unsigned char b,
                                     unsigned char a);

// Circles
void LIBCANVAS_PREFIX(ctx_stroke_circle)(struct canvas_ctx *ctx,
                                         int x, int y, int rad,
                                         unsigned char r,
                                         unsigned char g,
                                         unsigned char b,
                                         unsigned char a);

// implementation
#ifdef LIBCANVAS_IMPLEMENTATION

#define LIBCANVAS_PRIV(x) canv_impl_##x

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Core */

struct canvas_ctx {
  uint8_t *src;
  int width;
  int height;
  canvas_format format;
};

struct canvas_ctx *LIBCANVAS_PREFIX(ctx_create)(int width, int height, canvas_format format) {
  struct canvas_ctx *ctx = LIBCANVAS_MALLOC(sizeof(struct canvas_ctx));
  ctx->width = width;
  ctx->height = height;
  ctx->format = format;
  switch (format) {
    case LIBCANVAS_FORMAT_ARGB32: {
      size_t words = width * height;
      ctx->src = LIBCANVAS_MALLOC(words * sizeof(uint32_t));
      uint32_t *data = (uint32_t *)ctx->src;
      for(size_t i = 0; i < words; i++) {
        data[i] = 0xff000000;
      }
      break;
    }
    case LIBCANVAS_FORMAT_RGB24: {
      size_t bytes = width * height * sizeof(uint32_t);
      ctx->src = LIBCANVAS_MALLOC(bytes);
      memset(ctx->src, 0, bytes);
      break;
    }
    default: {
      LIBCANVAS_DEBUG("TODO: unsupported format %d\n", format);
      LIBCANVAS_FREE(ctx);
      return NULL;
    }
  }
  return ctx;
}

void LIBCANVAS_PREFIX(ctx_destroy)(struct canvas_ctx *ctx) {
  LIBCANVAS_FREE(ctx->src);
  LIBCANVAS_FREE(ctx);
}

uint8_t *LIBCANVAS_PREFIX(ctx_get_surface)(struct canvas_ctx *ctx) {
  return ctx->src;
}

canvas_format LIBCANVAS_PREFIX(canv_ctx_get_format)(struct canvas_ctx *ctx) {
  return ctx->format;
}

int LIBCANVAS_PREFIX(ctx_get_width)(struct canvas_ctx *ctx) {
  return ctx->width;
}

int LIBCANVAS_PREFIX(ctx_get_height)(struct canvas_ctx *ctx) {
  return ctx->height;
}

/* Colors */
static inline uint32_t
LIBCANVAS_PRIV(rgba_to_word)(struct canvas_ctx *ctx,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (ctx->format == LIBCANVAS_FORMAT_ARGB32)
    return a << 24 | r << 16 | g << 8 | b;
  else
    return r << 16 | g << 8 | b;
}

/* Rectangles */

void LIBCANVAS_PREFIX(ctx_fill_rect)(struct canvas_ctx *ctx,
                                     int xs, int ys, int width, int height,
                                     uint8_t r,
                                     uint8_t g,
                                     uint8_t b,
                                     uint8_t a) {
  // checks
  if(xs < 0) {
    xs = 0;
    width -= xs;
  } else if(xs > ctx->width) {
    return;
  }

  if(ys < 0) {
    ys = 0;
    height -= ys;
  } else if(ys > ctx->height) {
    return;
  }

  if(xs + width > ctx->width)
    width = ctx->width - xs;
  if(ys + height > ctx->height)
    height = ctx->height - ys;

  // blit
  switch (ctx->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      if (a == __UINT8_MAX__) {
        uint32_t color = LIBCANVAS_PRIV(rgba_to_word)(ctx, r, g, b, a);
        uint32_t *dst = (uint32_t *)ctx->src;
        for (int y = ys; y < (ys + height); y++) {
          for (int x = xs; x < (xs + width); x++) {
            dst[y * ctx->width + x] = color;
          }
        }
      } else {
        LIBCANVAS_DEBUG("TODO: unsupported alpha blitting\n");
      }
      break;
    }
    default: {
      LIBCANVAS_DEBUG("TODO: unsupported format %d\n", ctx->format);
      return;
    }
  }
}

/* Lines */

void LIBCANVAS_PREFIX(ctx_stroke_line)(struct canvas_ctx *ctx,
                                      int x0, int y0, int x1, int y1,
                                      uint8_t r,
                                      uint8_t g,
                                      uint8_t b,
                                      uint8_t a) {
  // TODO: checks

  int dx = x1 - x0;
  int dy = y1 - y0;
  int D = 2 * dy - dx;
  int y = y0;

  switch (ctx->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      uint32_t color = LIBCANVAS_PRIV(rgba_to_word)(ctx, r, g, b, a);
      uint32_t *dst = (uint32_t *)ctx->src;
      for (int x = x0; x < x1; x++) {
        dst[y * ctx->width + x] = color;
        if (D > 0) {
          y++;
          D -= 2 * dx;
        }
        D += 2 * dy;
      }
      break;
    }
    default: {
      LIBCANVAS_DEBUG("TODO: unsupported format %d\n", ctx->format);
      return;
    }
  }
}

/* Circles */
static void LIBCANVAS_PRIV(stroke_circle_oct)(struct canvas_ctx *ctx,
                                              int xc, int yc, int x, int y,
                                              unsigned char r,
                                              unsigned char g,
                                              unsigned char b,
                                              unsigned char a) {
  switch (ctx->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      uint32_t color = LIBCANVAS_PRIV(rgba_to_word)(ctx, r, g, b, a);
      uint32_t *dst = (uint32_t *)ctx->src;
#define _putpixel(x, y) dst[(y) * ctx->width + (x)] = color
      _putpixel(xc + x, yc + y);
      _putpixel(xc - x, yc + y);
      _putpixel(xc + x, yc - y);
      _putpixel(xc - x, yc - y);
      _putpixel(xc + y, yc + x);
      _putpixel(xc - y, yc + x);
      _putpixel(xc + y, yc - x);
      _putpixel(xc - y, yc - x);
#undef _putpixel
      break;
    }
  }
}

void LIBCANVAS_PREFIX(ctx_stroke_circle)(struct canvas_ctx *ctx,
                                         int xc, int yc, int rad,
                                         unsigned char r,
                                         unsigned char g,
                                         unsigned char b,
                                         unsigned char a) {
  if (xc + rad > ctx->width || xc - rad < 0)
    return;
  if (yc + rad > ctx->height || yc - rad < 0)
    return;

  int x = 0, y = rad;
  int d = 3 - 2 * rad;
  LIBCANVAS_PRIV(stroke_circle_oct)(ctx, xc, yc, x, y, r, g, b, a);
  while (y >= x) {
    x++;
    if (d > 0) {
      y--;
      d += 4 * (x - y) + 10;
    } else
      d += 4 * x + 6;
    LIBCANVAS_PRIV(stroke_circle_oct)(ctx, xc, yc, x, y, r, g, b, a);
  }
}

#endif