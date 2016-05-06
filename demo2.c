#include <math.h>
#define GFX_IMPL
#include "gfx.h"

static void *img;
static int ximg, yimg;
static u8 keys[256];
static float mx=G_XRES/2, my=G_YRES/2;
static float pt_x=G_XRES/2, pt_y=G_YRES/2;

static void draw(double t)
{
  v4 col = {0.2f, 0.2f, 0.4f, 0.0f};
  float ang = (float)t;
  g_clear(col);

  float xa = cosf(ang);
  float ya = sinf(ang);
  float xs = 1.25f + 0.75f*xa;
  float ys = xs;
  v4 axis = { xa*xs, -ya*xs, ya*ys, xa*ys };
  g_sprite(G_XRES/2, G_YRES/2, axis, 0, img, ximg, yimg, 0);

  /*v4_t col = { 1.0f, 0.0f, 1.0f, 1.0f };
  col[3] = 0.5f+0.5f*cosf(ang*2.0f);

  float lx = 300.0f*cosf(ang);
  float ly = 300.0f*sinf(ang);
  g_line(pt_x, pt_y, pt_x+lx, pt_y+ly, col);

  col[0] = 1.0f;
  col[2] = 0.0f;
  float sz = 256.0f;
  g_rect(mx, my, sz, sz, col);*/
}

int g_setup(void)
{
  img = g_readtga("test_32.tga", &ximg, &yimg);
  return 0;
}

int g_frame(double t, double dt)
{
  int ev, ex, ey;
  while ((ev = g_event(&ex, &ey)) != 0) {
    switch (ev) {
    case G_WINSHUT:
      return G_EXIT;
    case G_KEYDOWN:
      if (ex == GK_ESC) return G_EXIT;
      keys[ex] = 1;
      break;
    case G_KEYUP:
      keys[ex] = 0;
      break;
    case G_MOUSE:
      mx=(float)(ex*G_XRES/g_xwin);
      my=(float)(ey*G_YRES/g_ywin);
      break;
    }
  }

  if      (keys[GK_LEFT])  --pt_x;
  else if (keys[GK_RIGHT]) ++pt_x;
  if      (keys[GK_UP])    --pt_y;
  else if (keys[GK_DOWN])  ++pt_y;

  draw(t);
  /*g_delay(1);*/
  return 0;
}
