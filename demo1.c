// test program for gfx.h
#include <math.h>
#define GFX_IMPL
#include "gfx.h"

static void printkey(int k, int d)
{
#define X(x) case x: g_ods("key(%d): %s\n", d, #x); break
  switch (k) {
  X(GK_BACK); X(GK_TAB); X(GK_RET); X(GK_SPACE); X(GK_ESC); X(GK_CTRL);
  X(GK_SHIFT); X(GK_ALT); X(GK_UP); X(GK_DOWN); X(GK_LEFT); X(GK_RIGHT);
  X(GK_INS); X(GK_DEL); X(GK_HOME); X(GK_END); X(GK_PGUP); X(GK_PGDN);
  X(GK_F1); X(GK_F2); X(GK_F3); X(GK_F4); X(GK_F5); X(GK_F6); X(GK_F7);
  X(GK_F8); X(GK_F9); X(GK_F10); X(GK_F11); X(GK_F12); X(GK_M1); X(GK_M2);
  X(GK_M3); X(GK_M4); X(GK_M5);
  default: g_ods("key(%d): '%c'\n", d, k);
  }
#undef X
}

static void draw(double t)
{
  static float fxd = 1.0f / G_XRES;
  static float fyd = 1.0f / G_YRES;
  float fx, fy=0.0f, ft=(float)t;
  u32 *p = g_scrbuf;

  g_perfbgn(0);

  for (int py=0; py<G_YRES; ++py, fy+=fyd, p+=G_XRES) {
    float s = 0.3f*sinf(fy*10.0f + ft);
    fx = 0.0f;
    for (int px=0; px<G_XRES; ++px, fx+=fxd) {
      float r = fx;
      float g = fy;
      float b = fabsf(fx+s-0.5f);
      g += ceilf((1.0f-b)*5.0f) * 0.1f;
      if (g > 1.0f) g = 1.0f;
      int cr = (int)(r*255.0f);
      int cg = (int)(g*255.0f);
      int cb = 255;
      p[px] = (cr<<16) | (cg<<8) | cb;
    }
  }

  g_perfend(0, 1);
}

int g_setup(void)
{
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
      printkey(ex, 1);
      if (ex == GK_ESC) return G_EXIT;
      break;
    case G_KEYUP:
      printkey(ex, 0);
      break;
    case G_KEYCHAR:
      g_ods("char:");
      if (ex > 32 && ex < 128) g_ods(" '%c'", ex);
      g_ods(" [%02X]\n", ex);
      break;
    case G_MOUSE:
      g_ods("mouse: %4d,%4d\n", ex*G_XRES/g_xwin, ey*G_YRES/g_ywin);
      break;
    }
  }

  draw(t);

  return 0;
}
