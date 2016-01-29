// test program for gfx.h
#define GFX_C
#include "gfx.h"

int g_loop(double t, double dt)
{
  u32_t *p = g_fb, ms = (int)(t*1000.0);
  for (u32_t y=0; y<G_YRES; ++y, p+=G_XRES) {
    u32_t z = y*y + ms;
    for (u32_t x=0; x<G_XRES; ++x)
      p[x] = x*x + z;
  }

  return 1;
}
