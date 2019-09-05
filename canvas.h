#pragma once

// header
struct canvas_ctx;
#define LIBCANVAS_ALLOC(x) malloc(x)
#define LIBCANVAS_FREE(x) free(x)
#define LIBCANVAS_DEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define LIBCANVAS_PREFIX(x) canv_##x

typedef enum {
  LIBCANVAS_FORMAT_ARGB,
  LIBCANVAS_FORMAT_RGB,
} canvas_format;

struct canvas_ctx *LIBCANVAS_PREFIX(ctx_init)(int width, int height, canvas_format format);
void LIBCANVAS_PREFIX(ctx_destroy)(struct canvas_ctx *);

// Rectangles
void LIBCANVAS_PREFIX(ctx_fill_rect)(struct canvas_ctx *ctx,
                                     int width, int height,
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

struct canvas_ctx {
  uint8_t *src;
  int width;
  int height;
  canvas_format format;
};

struct canvas_ctx *LIBCANVAS_PREFIX(ctx_init)(int width, int height, canvas_format format) {
  struct canvas_ctx *ctx = LIBCANVAS_ALLOC(sizeof(struct canvas_ctx));
  ctx->width = width;
  ctx->height = height;
  ctx->format = format;
  switch (format) {
    case LIBCANVAS_FORMAT_ARGB:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB:
      ctx->src = LIBCANVAS_ALLOC(width * height * 4);
      break;
    default:
      LIBCANVAS_DEBUG("TODO: unsupported format %d\n", format);
      LIBCANVAS_FREE(ctx);
      return NULL;
  }
  return ctx;
}

void LIBCANVAS_PREFIX(ctx_destroy)(struct canvas_ctx *ctx) {
  free(ctx->src);
  free(ctx);
}

#pragma region Rectangles

void LIBCANVAS_PREFIX(ctx_fill_rect)(struct canvas_ctx *ctx,
                                     int width, int height,
                                     uint8_t r,
                                     uint8_t g,
                                     uint8_t b,
                                     uint8_t a) {
  switch (ctx->format) {
    case LIBCANVAS_FORMAT_ARGB:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB: {
      if (a == __UINT8_MAX__) {
          uint32_t color = r << 16 | g << 8 | b;
          for (int y = 0; y < height; y++) {
              for (int x = 0; x < width; x++) {
                  ctx->src[x * y] = color;
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

#pragma endregion

#endif