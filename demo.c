#include <math.h>
#define USE_SSE
#define GFX_C
#include "gfx.h"

static u8_t keys[256];
static float mx=G_XRES/2, my=G_YRES/2;
static float pt_x=G_XRES/2, pt_y=G_YRES/2;

static void draw(double t)
{
  float ang = (float)t;
  g_clear((v4_t){0.2f, 0.2f, 0.4f, 0.0f});

  v4_t col = { 1.0f, 0.0f, 1.0f, 1.0f };

  float lx = 300.0f*cosf(ang);
  float ly = 300.0f*sinf(ang);
  g_line(pt_x, pt_y, pt_x+lx, pt_y+ly, col);

  col[0] = 1.0f;
  col[2] = 0.0f;
  col[3] = 0.5f+0.5f*cosf(ang*2.0f);
  float sz = 256.0f;
  g_rect(mx, my, sz, sz, col);
}

int g_loop(double t, double dt)
{
  u32_t ev, ep;
  while ((ev = g_event(&ep)) != 0) {
    switch (ev) {
    case GE_QUIT:
      return 0;
    case GE_KEYDOWN:
      if (ep == GK_ESC) return 0;
      keys[ep] = 1;
      break;
    case GE_KEYUP:
      keys[ep] = 0;
      break;
    case GE_MOUSE:
      mx=(float)((ep&0xFFFF)*G_XRES/g_dw);
      my=(float)((ep>>16)*G_YRES/g_dh);
      break;
    }
  }

  if      (keys[GK_LEFT])  --pt_x;
  else if (keys[GK_RIGHT]) ++pt_x;
  if      (keys[GK_UP])    --pt_y;
  else if (keys[GK_DOWN])  ++pt_y;

  draw(t);
  /*g_delay(1);*/
  return 1;
}
