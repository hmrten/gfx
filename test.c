// test program for gfx.h
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

static void printchar(int key)
{
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
int g_loop(double t, double dt)
{
  if (!events())
    return 0;

  u32_t *p = g_fb, ms = (int)(t*1000.0);
  for (u32_t y=0; y<G_YRES; ++y, p+=G_XRES) {
    u32_t z = y*y + ms;
    for (u32_t x=0; x<G_XRES; ++x)
      p[x] = x*x + z;
  }

  return 1;
}
