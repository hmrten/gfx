#include <math.h>
#define GFX_C
#include "gfx.h"

static void draw(double t)
{
  g_clear((v4_t){0.5f, 0.5f, 0.5f, 1.0f});
}

int g_loop(double t, double dt)
{
  u32_t ev, ep;
  while ((ev = g_event(&ep)) != 0) {
    if (ev == GE_QUIT || (ev == GE_KEYDOWN && ep == GK_ESC))
      return 0;
  }

  draw(t);
  return 1;
}
