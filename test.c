// test program for gfx.h
#include <math.h>
#define GFX_C
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

static int events(void)
{
  u32_t ev, ep;
  while ((ev = g_event(&ep)) != 0) {
    switch (ev) {
    case GE_QUIT:    return 0;
    case GE_KEYDOWN: printkey(ep, 1);
                     if (ep == GK_ESC) return 0;
                     break;
    case GE_KEYUP:   printkey(ep, 0); break;
    case GE_KEYCHAR: g_ods("char:");
                     if (ep > 32 && ep < 128) g_ods(" '%c'", ep);
                     g_ods(" [%02X]\n", ep);
                     break;
    case GE_MOUSE:   g_ods("mouse: %4d,%4d\n",
                           (ep&0xFFFF)*G_XRES/g_dw, (ep>>16)*G_YRES/g_dh);
                     break;
    }
  }
  return 1;
}

static void draw(double t)
{
  static float fxd = 1.0f / G_XRES;
  static float fyd = 1.0f / G_YRES;
  float fx, fy=0.0f, ft=(float)t;
  u32_t *p = g_fb;
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
}

int g_loop(double t, double dt)
{
  if (!events())
    return 0;

  draw(t);
  return 1;
}
