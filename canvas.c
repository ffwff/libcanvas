#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "canvas.h"

/* Optimized functions */

static inline void memset_long(uint32_t *dst, uint32_t c, size_t words) {
  unsigned int d0, d1, d2;
  __asm__ __volatile__(
    "cld\nrep stosl"
    : "=&a"(d0), "=&D"(d1), "=&c"(d2)
    : "0"(c),
      "1"(dst),
      "2"(words)
    : "memory");
}

static inline void memcpy_long(uint32_t *dst, uint32_t *src, size_t words) {
  unsigned int d0, d1, d2;
  __asm__ __volatile__(
    "cld\nrep movsl"
    : "=&S"(d0), "=&D"(d1), "=&c"(d2)
    : "0"(src),
      "1"(dst),
      "2"(words)
    : "memory");
}

#if defined(__SSE2__)
#include <emmintrin.h>
static inline void
mask_color_long(uint32_t *udst, uint32_t *usrc, uint32_t color, size_t words) {
  __m128i mask = _mm_setr_epi32(color, color, color, color);
  for(size_t i = 0; i < words; i += 4) {
    __m128i *dst = (__m128i *)(&udst[i]);
    __m128i src = _mm_loadu_si128((__m128i *)(&usrc[i]));
    __m128i cmp = _mm_cmpeq_epi32(src, mask);
    __m128i s1 = _mm_and_si128(cmp, _mm_loadu_si128(dst));
    __m128i s2 = _mm_andnot_si128(cmp, src);
    _mm_storeu_si128(dst, _mm_or_si128(s1, s2));
  }
}
#else
static inline void
mask_color_long(uint32_t *udst, uint32_t *usrc, uint32_t color, size_t words) {
  for(size_t i = 0; i < words; i++) {
    uint32_t cmp = usrc[i] == color;
    uint32_t s1 = udst[i] & cmp;
    uint32_t s2 = usrc[i] & ~cmp;
    udst[i] = s1 | s2;
  }
}
#endif


/* Core */

struct canvas_ctx {
  uint8_t *src;
  int width;
  int height;
  canvas_format format;
  int free_src;
};

struct canvas_ctx *LIBCANVAS_PREFIX(ctx_create)(int width, int height, canvas_format format) {
  struct canvas_ctx *ctx = LIBCANVAS_MALLOC(sizeof(struct canvas_ctx));
  ctx->width = width;
  ctx->height = height;
  ctx->format = format;
  ctx->free_src = 1;
  switch (format) {
    case LIBCANVAS_FORMAT_ARGB32: {
      size_t words = width * height;
      ctx->src = LIBCANVAS_MALLOC(words * sizeof(uint32_t));
      uint32_t *data = (uint32_t *)ctx->src;
      memset_long(data, 0xff000000, words);
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

struct canvas_ctx *LIBCANVAS_PREFIX(ctx_create_from_surface)
  (void *ptr, int width, int height, canvas_format format) {
  struct canvas_ctx *ctx = LIBCANVAS_MALLOC(sizeof(struct canvas_ctx));
  ctx->width = width;
  ctx->height = height;
  ctx->format = format;
  ctx->src = (uint8_t *)ptr;
  ctx->free_src = 0;
  return ctx;
}

void LIBCANVAS_PREFIX(ctx_destroy)(struct canvas_ctx *ctx) {
  if(ctx->free_src)
    LIBCANVAS_FREE(ctx->src);
  LIBCANVAS_FREE(ctx);
}

uint8_t *LIBCANVAS_PREFIX(ctx_get_surface)(struct canvas_ctx *ctx) {
  return ctx->src;
}

canvas_format LIBCANVAS_PREFIX(ctx_get_format)(struct canvas_ctx *ctx) {
  return ctx->format;
}

int LIBCANVAS_PREFIX(ctx_get_width)(struct canvas_ctx *ctx) {
  return ctx->width;
}

int LIBCANVAS_PREFIX(ctx_get_height)(struct canvas_ctx *ctx) {
  return ctx->height;
}

int LIBCANVAS_PREFIX(ctx_resize_buffer)(struct canvas_ctx *ctx, int width, int height) {
  if(width < 0 || height < 0)
    return 1;
  ctx->width = width;
  ctx->height = height;
  switch (ctx->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      size_t bytes = width * height * sizeof(uint32_t);
      ctx->src = LIBCANVAS_REALLOC(ctx->src, bytes);
      return ctx->src != NULL;
    }
    default: {
      LIBCANVAS_DEBUG("TODO: unsupported format %d\n", ctx->format);
      return 1;
    }
  }
}

/* Colors */
static inline uint32_t
rgba_to_word(struct canvas_ctx *ctx,
             struct canvas_color color) {
  if (ctx->format == LIBCANVAS_FORMAT_ARGB32)
    return color.a << 24 | color.r << 16 | color.g << 8 | color.b;
  else
    return color.r << 16 | color.g << 8 | color.b;
}

/* Rectangles */

void LIBCANVAS_PREFIX(ctx_fill_rect)(struct canvas_ctx *ctx,
                                     int xs, int ys, int width, int height,
                                     struct canvas_color ccolor) {
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
      uint32_t color = rgba_to_word(ctx, ccolor);
      uint32_t *dst = (uint32_t *)ctx->src;
      for (int y = ys; y < (ys + height); y++) {
        memset_long(dst + y * ctx->width + xs, color, width);
      }
      break;
    }
    default: {
      LIBCANVAS_DEBUG("TODO: unsupported format %d\n", ctx->format);
      return;
    }
  }
}

void LIBCANVAS_PREFIX(ctx_stroke_rect)(struct canvas_ctx *ctx,
                                     int xs, int ys, int width, int height,
                                     struct canvas_color color) {
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xs, ys, width, 1, color);
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xs, ys, 1, height, color);
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xs, ys + height, width, 1, color);
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xs + width, ys, 1, height, color);
}
/* Lines */

void LIBCANVAS_PREFIX(ctx_stroke_line)(struct canvas_ctx *ctx,
                                      int x0, int y0, int x1, int y1,
                                      struct canvas_color ccolor) {
  // TODO: checks

  if(y0 == y1) {
    if(x0 < x1) {
      LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, x0, y0, x1 - x0, 1, ccolor);
    } else {
      LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, x1, y0, x0 - x1, 1, ccolor);
    }
    return;
  } else if(x0 == x1) {
    if(y0 < y1) {
      LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, x0, y0, 1, y1 - y0, ccolor);
    } else {
      LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, x0, y1, 1, y0 - y1, ccolor);
    }
    return;
  }

  int dx = x1 - x0;
  int dy = y1 - y0;
  int D = 2 * dy - dx;
  int y = y0;

  switch (ctx->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      uint32_t color = rgba_to_word(ctx, ccolor);
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
static void stroke_circle_oct(struct canvas_ctx *ctx,
                              int xc, int yc, int x, int y,
                              struct canvas_color ccolor) {
  switch (ctx->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      uint32_t color = rgba_to_word(ctx, ccolor);
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

static void fill_circle_oct(struct canvas_ctx *ctx,
                            int xc, int yc, int x, int y,
                            struct canvas_color ccolor) {
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xc - x, yc + y, x * 2, 1, ccolor);
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xc - x, yc - y, x * 2, 1, ccolor);
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xc - y, yc + x, y * 2, 1, ccolor);
  LIBCANVAS_PREFIX(ctx_fill_rect)(ctx, xc - y, yc - x, y * 2, 1, ccolor);
}

typedef void (*circle_oct_fn)(struct canvas_ctx *ctx,
                              int xc, int yc, int x, int y,
                              struct canvas_color ccolor);

static inline void ctx_circle_wrapper(struct canvas_ctx *ctx,
                                      int xc, int yc, int rad,
                                      struct canvas_color ccolor,
                                      circle_oct_fn oct_fn) {
  if (xc + rad > ctx->width || xc - rad < 0)
    return;
  if (yc + rad > ctx->height || yc - rad < 0)
    return;

  int x = 0, y = rad;
  int d = 3 - 2 * rad;
  oct_fn(ctx, xc, yc, x, y, ccolor);
  while (y >= x) {
    x++;
    if (d >= 0) {
      y--;
      d += 4 * (x - y) + 10;
    } else
      d += 4 * x + 6;
    oct_fn(ctx, xc, yc, x, y, ccolor);
  }
}

void LIBCANVAS_PREFIX(ctx_stroke_circle)(struct canvas_ctx *ctx,
                                         int xc, int yc, int rad,
                                         struct canvas_color color) {
  ctx_circle_wrapper(ctx, xc, yc, rad, color, stroke_circle_oct);
}

void LIBCANVAS_PREFIX(ctx_fill_circle)(struct canvas_ctx *ctx,
                                        int xc, int yc, int rad,
                                        struct canvas_color color) {
  ctx_circle_wrapper(ctx, xc, yc, rad, color, fill_circle_oct);
}

/* Bit blit */
void LIBCANVAS_PREFIX(ctx_bitblit)(struct canvas_ctx *dst,
                                   struct canvas_ctx *src,
                                   int dx, int dy) {
  switch (dst->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      if (src->format != LIBCANVAS_FORMAT_ARGB32 && src->format != LIBCANVAS_FORMAT_RGB24)
        return;
      for(int y = 0; y < src->height; y++) {
        uintptr_t offset = (dy + y) * dst->width + dx;
        uintptr_t src_offset = y * src->width;
        uint32_t *dst_surf = (uint32_t *)dst->src;
        uint32_t *src_surf = (uint32_t *)src->src;
        memcpy_long(&dst_surf[offset], &src_surf[src_offset], src->width);
      }
      break;
    }
    default: {
      // TODO
    }
  }
}

/* Bit blit */
void LIBCANVAS_PREFIX(ctx_bitblit_mask)(struct canvas_ctx *dst,
                                        struct canvas_ctx *src,
                                        int dx, int dy,
                                        struct canvas_color ccolor) {
  switch (dst->format) {
    case LIBCANVAS_FORMAT_ARGB32:
      // fallthrough
    case LIBCANVAS_FORMAT_RGB24: {
      if (src->format != LIBCANVAS_FORMAT_ARGB32 && src->format != LIBCANVAS_FORMAT_RGB24)
        return;
      uint32_t color = rgba_to_word(src, ccolor);
      for(int y = 0; y < src->height; y++) {
        uintptr_t offset = (dy + y) * dst->width + dx;
        uintptr_t src_offset = y * src->width;
        uint32_t *dst_surf = (uint32_t *)dst->src;
        uint32_t *src_surf = (uint32_t *)src->src;
        mask_color_long(&dst_surf[offset], &src_surf[src_offset], color, src->width);
      }
      break;
    }
    default: {
      // TODO
    }
  }
}
