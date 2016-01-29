#include <math.h>
#define GFX_C
#include "gfx.h"

static float mx=G_XRES/2, my=G_YRES/2;

static void draw(double t)
{
  float ang = (float)t;
  g_clear((v4_t){0.5f, 0.5f, 0.5f, 0.0f});

  v4_t col = { 1.0f, 0.0f, 0.0f };
  col[3] = 0.5f+0.5f*cosf(ang*2.0f);
  g_rect(mx-64.0f, my-64.0f, 128.0f, 128.0f, col);
}

int g_loop(double t, double dt)
{
  u32_t ev, ep;
  while ((ev = g_event(&ep)) != 0) {
    if (ev == GE_QUIT || (ev == GE_KEYDOWN && ep == GK_ESC))
      return 0;
    else if (ev == GE_MOUSE)
      mx=(float)((ep&0xFFFF)*G_XRES/g_dw), my=(float)((ep>>16)*G_YRES/g_dh);
  }

  draw(t);
  g_delay(1);
  return 1;
}
