/* gfx.h
 * Written in 2016 by Mårten Hansson <hmrten@gmail.com> */

#ifndef G_XRES
#define G_XRES 640
#endif
#ifndef G_YRES
#define G_YRES 480
#endif

enum {
  /* return codes */
  G_EXIT=1,
  /* events */
  G_WINSHUT = 1, G_KEYDOWN, G_KEYUP, G_KEYCHAR, G_MOUSE,
  /* keycodes ('0'..'9' and 'a'..'z' as lowercase ascii) */
  GK_BACK=8, GK_TAB=9, GK_RET=13, GK_ESC=27, GK_SPACE=32, GK_CTRL=128,
  GK_SHIFT, GK_ALT, GK_UP, GK_DOWN, GK_LEFT, GK_RIGHT, GK_INS, GK_DEL,
  GK_HOME, GK_END, GK_PGUP, GK_PGDN, GK_F1, GK_F2, GK_F3, GK_F4, GK_F5,
  GK_F6, GK_F7, GK_F8, GK_F9, GK_F10, GK_F11, GK_F12, GK_M1, GK_M2, GK_M3,
  GK_M4, GK_M5
};

typedef signed   __int8   s8;
typedef unsigned __int8   u8;
typedef signed   __int16 s16;
typedef unsigned __int16 u16;
typedef signed   __int32 s32;
typedef unsigned __int32 u32;
typedef signed   __int64 s64;
typedef unsigned __int64 u64;

typedef __declspec(align(16)) float v4[4];

extern void *g_pixels;
extern u32   g_ticks;
extern u32   g_xwin;
extern u32   g_ywin;
extern u64   g_perf[64][2];

#define g_assert(p) do { if (!(p)) *(volatile char *)0 = 0xCC; } while(0)

#define g_perfbgn(i)    (g_perf[i][0] = __rdtsc())
#define g_perfend(i, n) (g_perf[i][0] = __rdtsc() - g_perf[i][0], \
                         g_perf[i][1] = (n))

#define g_min(a, b) ((a) < (b) ? (a) : (b))
#define g_max(a, b) ((a) > (b) ? (a) : (b))

int   g_event(int *, int *);
void  g_winsize(int, int);
void  g_wintext(const char *);
void  g_delay(int);
void  g_ods(const char *, ...);
void *g_readtga(const char *, int *, int *);
void  g_clear(v4);
void  g_rect(float, float, float, float, v4);
void  g_line(float, float, float, float, v4);
void  g_sprite(float, float, v4, v4, const void *, int, int, int *);

int   g_setup(void);
int   g_frame(double, double);

#ifdef GFX_IMPL
#include <stdlib.h>
#include <stdio.h>
#include <intrin.h>

void *g_pixels;
u32   g_ticks;
u32   g_xwin;
u32   g_ywin;
u64   g_perf[64][2];

static const float g_xmaxf = (float)(G_XRES-1);
static const float g_ymaxf = (float)(G_YRES-1);

static __inline float v4_min(v4 v)
{
  __m128 x = _mm_set_ss(v[0]);
  x = _mm_min_ss(x, _mm_set_ss(v[1]));
  x = _mm_min_ss(x, _mm_set_ss(v[2]));
  x = _mm_min_ss(x, _mm_set_ss(v[3]));
  return _mm_cvtss_f32(x);
}

static __inline float v4_max(v4 v)
{
  __m128 x = _mm_set_ss(v[0]);
  x = _mm_max_ss(x, _mm_set_ss(v[1]));
  x = _mm_max_ss(x, _mm_set_ss(v[2]));
  x = _mm_max_ss(x, _mm_set_ss(v[3]));
  return _mm_cvtss_f32(x);
}

static __inline __m128 ps_fromint(u32 i)
{
  __m128i sm = _mm_set_epi32(0x80808003, 0x80808000, 0x80808001, 0x80808002);
  __m128i pi = _mm_shuffle_epi8(_mm_cvtsi32_si128(i), sm);
  return _mm_mul_ps(_mm_cvtepi32_ps(pi), _mm_set1_ps(1.0f/255.0f));
}

static __inline u32 ps_toint(__m128 ps)
{
  __m128i pi = _mm_cvttps_epi32(_mm_mul_ps(ps, _mm_set1_ps(255.0f)));
  __m128i zi = _mm_setzero_si128();
  pi = _mm_shuffle_epi32(pi, 0xC6);
  pi = _mm_packus_epi16(_mm_packs_epi32(pi, zi), zi);
  return _mm_cvtsi128_si32(pi);
}

#define ps_madd(x, y, z) _mm_add_ps(_mm_mul_ps(x, y), z)
#define ps_scale1(p, s) _mm_mul_ps(p, _mm_set1_ps(s))

void *g_readtga(const char *path, int *ximg, int *yimg)
{
  u8 hdr[18], type, depth;
  size_t w, h, n;
  FILE *f = fopen(path, "rb");

  g_assert(f);

  n = fread(hdr, 1, sizeof hdr, f);
  g_assert(n == sizeof hdr);

  type = hdr[0x02];
  w = (hdr[0x0D] << 8) | hdr[0x0C];
  h = (hdr[0x0F] << 8) | hdr[0x0E];
  depth = hdr[0x10];

  g_assert(type == 2 && depth == 32);

  void *img = malloc(w*h*4);
  g_assert(img);

  n = fread(img, 1, w*h*4, f);
  g_assert(n == w*h*4);
  fclose(f);

  *ximg = (int)w;
  *yimg = (int)h;

  return img;
}

void g_clear(v4 col)
{
  __m128 ps = _mm_load_ps(col);
  __stosd(g_pixels, ps_toint(ps), G_XRES*G_YRES);
}

void g_rect(float x, float y, float w, float h, v4 col)
{
  g_perfbgn(0);

  float halfw = 0.5f*w;
  float halfh = 0.5f*h;

  int ix0 = (int)(x-halfw);
  int ix1 = (int)(x+halfw);
  int iy0 = (int)(y-halfh);
  int iy1 = (int)(y+halfh);

  if (ix0 < 0) ix0 = 0;
  if (ix1 > G_XRES) ix1 = G_XRES;
  if (iy0 < 0) iy0 = 0;
  if (iy1 > G_YRES) iy1 = G_YRES;

  int iw = ix1-ix0;
  int ih = iy1-iy0;

  if (iw <= 0 || ih <= 0) {
    g_perfend(0, 1);
    return;
  }

  g_assert(ix0 >= 0 && ix0 < G_XRES);
  g_assert(ix1 >= 0 && ix1 <= G_XRES);
  g_assert(iy0 >= 0 && iy0 < G_YRES);
  g_assert(iy1 >= 0 && iy1 <= G_YRES);

  float t = col[3];
  __m128 m = _mm_set1_ps(1.0f-t);
  __m128 a = ps_scale1(_mm_load_ps(col), t);
  u32 *p = (u32 *)g_pixels + iy0*G_XRES + ix0;
  for (int y=0; y<ih; ++y) {
    u32 *xp = p;
    for (int x=0; x<iw; ++x)
      *xp++ = ps_toint(ps_madd(ps_fromint(p[x]), m, a));
    p += G_XRES;
  }

  g_perfend(0, iw*ih);
}

static int g_linetest(float p, float q, float *t)
{
  float r = q / p;
  if (p==0.0f && q<0.0f) return 0;
  if (p < 0.0f) {
    if      (r > t[1]) return 0;
    else if (r > t[0]) t[0] = r;
  } else if (p > 0.0f) {
    if      (r < t[0]) return 0;
    else if (r < t[1]) t[1] = r;
  }
  return 1;
}

static int g_clipline(float *x0, float *y0, float *x1, float *y1)
{
  float x = *x0;
  float y = *y0;
  float dx = *x1 - x;
  float dy = *y1 - y;
  float t[2] = { 0.0f, 1.0f };
  if (!g_linetest(-dx, x, t)) return 0;
  if (!g_linetest( dx, g_xmaxf-x, t)) return 0;
  if (!g_linetest(-dy, y, t)) return 0;
  if (!g_linetest( dy, g_ymaxf-y, t)) return 0;
  *x0 = x + dx*t[0];
  *y0 = y + dy*t[0];
  *x1 = x + dx*t[1];
  *y1 = y + dy*t[1];
  return 1;
}

void g_line(float x0, float y0, float x1, float y1, v4 col)
{
  if (!g_clipline(&x0, &y0, &x1, &y1))
    return;

  int ix0 = (int)x0;
  int ix1 = (int)x1;
  int iy0 = (int)y0;
  int iy1 = (int)y1;

  g_assert(ix0 >= 0 && ix0 < G_XRES);
  g_assert(ix1 >= 0 && ix1 < G_XRES);
  g_assert(iy0 >= 0 && iy0 < G_YRES);
  g_assert(iy1 >= 0 && iy1 < G_YRES);

  int dx = ix1-ix0;
  int dy = iy1-iy0;
  size_t e0 = dx > 0 ? 1 : -1;
  size_t e1 = e0;
  size_t step0 = dy > 0 ? G_XRES : -G_XRES;
  size_t step1 = 0;
  int i = dx > 0 ? dx : -dx;
  int j = dy > 0 ? dy : -dy;
  int d, n;

  if (j >= i)
    e1 = 0, step1 = step0, d = i, i = j, j = d;
  d = i / 2;
  step0 += e0;
  step1 += e1;
  n = i;

  float t = col[3];
  __m128 m = _mm_set1_ps(1.0f-t);
  __m128 a = ps_scale1(_mm_load_ps(col), t);
  u32 *p = (u32 *)g_pixels + iy0*G_XRES + ix0;
  do {
    *p = ps_toint(ps_madd(ps_fromint(*p), m, a));
    d += j;
    if (d >= i)
      d -= i, p += step0;
    else
      p += step1;
  } while (n--);
}

void g_sprite(float x, float y, v4 axis, v4 tint,
              const void *img, int ximg, int yimg, int *rimg)
{
  g_perfbgn(0);

  float fximg = (float)ximg;
  float fyimg = (float)yimg;

  axis[0] *= fximg;
  axis[2] *= fximg;
  axis[1] *= fyimg;
  axis[3] *= fyimg;

  x -= 0.5f*(axis[0]+axis[2]);
  y -= 0.5f*(axis[1]+axis[3]);

  v4 bx = { x, x+axis[0], x+axis[0]+axis[2], x+axis[2] };
  v4 by = { y, y+axis[1], y+axis[1]+axis[3], y+axis[3] };
  int xmin = (int)v4_min(bx);
  int xmax = (int)v4_max(bx);
  int ymin = (int)v4_min(by);
  int ymax = (int)v4_max(by);
  if (xmin < 0) xmin = 0;
  if (ymin < 0) ymin = 0;
  if (xmax > G_XRES-1) xmax = G_XRES-1;
  if (ymax > G_YRES-1) ymax = G_YRES-1;

  float da01 = 1.0f / sqrtf(axis[0]*axis[0] + axis[1]*axis[1]);
  float da23 = 1.0f / sqrtf(axis[2]*axis[2] + axis[3]*axis[3]);
  da01 *= da01;
  da23 *= da23;
  float dx0 = da01*axis[0];
  float dx1 = da01*axis[2];
  float dy0 = da23*axis[1];
  float dy1 = da23*axis[3];
  float sx = (float)xmin - x;
  float sy = (float)ymin - y;
  float ur = sx*dx0 + sy*dy0;
  float vr = sx*dx1 + sy*dy1;

  int w = xmax-xmin+1;
  int h = ymax-ymin+1;

  u32 *pimg = (u32 *)img;
  u32 *prow = (u32 *)g_pixels + ymin*G_XRES + xmin;
  u32 *plast = (u32 *)g_pixels + ymax*G_XRES;
  while (prow <= plast) {
    float u = ur;
    float v = vr;
    for (int i=0; i<w; ++i) {
      if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
        u32 ix = (int)(u*fximg);
        u32 iy = (int)(v*fyimg);
        u32 ipix = pimg[iy*(u32)ximg+ix];
        float t = (float)(ipix>>24)*(1.0f/255.0f);
        __m128 m = _mm_set1_ps(1.0f-t);
        __m128 a = ps_scale1(ps_fromint(ipix), t);
        prow[i] = ps_toint(ps_madd(ps_fromint(prow[i]), m, a));
      } else {
        //prow[i] = 0xCCCCCC;
      }
      u += dx0;
      v += dx1;
    }
    ur += dy0;
    vr += dy1;
    prow += G_XRES;
  }

  g_perfend(0, w*h);
}

#ifdef GFX_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

static u32    i_pixels[G_XRES*G_YRES];
static HWND   i_hw;
static HDC    i_dc;
static u32    i_winsize;
static u32    i_qhead;
static u32    i_qdata[256][2];
static u32    i_qtail;
static HANDLE i_heap;

static int i_addevent(u32 ev, u32 ep)
{
  u32 head = i_qhead, tail = i_qtail, ntail = (tail+1) & 255;
  _ReadWriteBarrier();
  if (ntail == head) return 0;
  i_qdata[tail][0] = ev;
  i_qdata[tail][1] = ep;
  _ReadWriteBarrier();
  i_qtail = ntail;
  return 1;
}

int g_event(int *ex, int *ey)
{
  u32 head = i_qhead, tail = i_qtail, ev, ep;
  _ReadWriteBarrier();
  if (head == tail) return 0;
  ev = i_qdata[head][0];
  ep = i_qdata[head][1];
  _ReadWriteBarrier();
  i_qhead = (head+1) & 255;
  *ex = (int)(ep&0xFFFF);
  *ey = (int)(ep>>16);
  return (int)ev;
}

void g_winsize(int w, int h)
{
  HWND hw = i_hw;
  RECT r = {0, 0, w, h};
  _ReadWriteBarrier();
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
  SetWindowPos(hw, HWND_TOP, 0, 0, r.right-r.left, r.bottom-r.top,
               SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
}

void g_wintext(const char *text)
{
  HWND hw = i_hw;
  _ReadWriteBarrier();
  SetWindowTextA(hw, text);
}

void g_delay(int ms)
{
  Sleep((DWORD)ms);
}

void g_ods(const char *fmt, ...)
{
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  OutputDebugStringA(buf);
  va_end(args);
}

static LRESULT CALLBACK i_winproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
  static const u8 vkmap[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, '\b', '\t', 0, 0, 0, '\r', 0, 0, GK_SHIFT,
    GK_CTRL, GK_ALT, 0, 0, 0, 0, 0, 0, 0, 0, GK_ESC, 0, 0, 0, 0, ' ',
    GK_PGUP, GK_PGDN, GK_END, GK_HOME, GK_LEFT, GK_UP, GK_RIGHT,
    GK_DOWN, 0, 0, 0, 0, GK_INS, GK_DEL, 0, '0', '1', '2', '3', '4', '5',
    '6', '7', '8', '9', 0, 0, 0, 0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, GK_F1, GK_F2, GK_F3, GK_F4, GK_F5, GK_F6,
    GK_F7, GK_F8, GK_F9, GK_F10, GK_F11, GK_F12
  };
  u32 ev=0, ep=0;
  switch (msg) {
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:  if ((lp&(1<<29)) && !(lp&(1<<31)) && wp == VK_F4) {
                         PostQuitMessage(0);
                         return 0;
                       }
  case WM_KEYUP:
  case WM_KEYDOWN:     if (!(lp&(1<<30)) == !!(lp&(1<<31))) return 0;
                       ev = (lp&(1<<31)) ? G_KEYUP : G_KEYDOWN;
                       if ((ep = vkmap[wp & 255])) goto event;
                       return 0;
  case WM_CHAR:        ev = G_KEYCHAR; ep = (u32)wp; goto event;
  case WM_LBUTTONUP:   ep = GK_M1; goto mup;
  case WM_MBUTTONUP:   ep = GK_M2; goto mup;
  case WM_RBUTTONUP:   ep = GK_M3; goto mup;
  case WM_LBUTTONDOWN: ep = GK_M1; goto mdown;
  case WM_MBUTTONDOWN: ep = GK_M2; goto mdown;
  case WM_RBUTTONDOWN: ep = GK_M3; goto mdown;
  case WM_MOUSEWHEEL:  ep = ((int)wp>>16) / WHEEL_DELTA > 0 ? GK_M4 : GK_M5;
                       i_addevent(G_KEYDOWN, ep);
                       i_addevent(G_KEYUP, ep);
                       return 0;
  case WM_MOUSEMOVE:   ev = G_MOUSE;
                       ep = (u32)lp;
                       goto event;
  case WM_SIZE:       _ReadWriteBarrier(); i_winsize = (u32)lp; return 0;
  case WM_ERASEBKGND: return 1;
  case WM_CLOSE:      PostQuitMessage(0); return 0;
  }
  return DefWindowProc(hw, msg, wp, lp);
mup:
  ev = G_KEYUP; goto event;
mdown:
  ev = G_KEYDOWN;
event:
  i_addevent(ev, ep);
  return 0;
}

static DWORD WINAPI i_winloop(void *arg)
{
  static WNDCLASS wc;
  RECT r = { 0, 0, G_XRES, G_YRES };
  MSG msg;

  wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
  wc.lpfnWndProc = i_winproc;
  wc.hInstance = GetModuleHandle(0);
  wc.hIcon = LoadIcon(0, IDI_APPLICATION);
  wc.hCursor = LoadCursor(0, IDC_ARROW);
  wc.lpszClassName = "gfx";
  RegisterClass(&wc);

  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
  r.right -= r.left, r.bottom -= r.top;
  i_hw = CreateWindow("gfx", "gfx", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                      CW_USEDEFAULT, CW_USEDEFAULT, r.right, r.bottom,
                      0, 0, wc.hInstance, 0);
  _ReadWriteBarrier();
  i_dc = GetDC(i_hw);

  while (GetMessage(&msg, 0, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  i_addevent(G_WINSHUT, 0);
  return 0;
}

static void i_dbgtime(HWND hw, double dt, double lt, double t)
{
  static char unit = 'M';
  static char str[256];
  static double ddt, ddt100, cdt, clt=1.0, ccy;
  static u32 ccyd;
  static int fps, cfps;

  ++fps;
  ddt += dt;
  if (ddt >= 1.0) {
    cdt = dt*1000.0;
    clt = lt*1000.0;
    cfps = (int)(fps / ddt);
    u64 cy = g_perf[0][0];
    ccyd = (u32)(cy / g_perf[0][1]);

    if (cy < 100000) unit = 'K', ccy = cy*1e-3;
    else unit = 'M', ccy = cy*1e-6;

    fps = 0;
    ddt = 0.0;
  }

  ddt100 += dt;
  if (ddt >= 0.01) {
    int m = (int)(t/60.0);
    int s = (int)(t-m*60.0);
    int c = (int)(t*100.0)%100;
    sprintf(str,
            "%02d:%02d:%02d - %.2f ms / %4d fps (loop: %.2f ms / %4d fps)"
            " (cycles: %.3f%c / %llu)",
            m, s, c, cdt, cfps, clt, (int)(1000.0/clt), ccy, unit, ccyd);
    SetWindowText(hw, str);
    ddt100 = 0.0;
  }
}

int WinMain(HINSTANCE inst, HINSTANCE pinst, LPSTR cmdline, int cmdshow)
{
  static BITMAPINFO bi = { sizeof(BITMAPINFOHEADER), G_XRES, -G_YRES, 1, 32 };
  LARGE_INTEGER tbase, tnow, tlast = { 0 }, tloop;
  double freq, t, dt, lt;
  HWND  hw;
  HDC   dc;
  u32   dw, dh;

  g_pixels = i_pixels;
  i_heap = GetProcessHeap();
  timeBeginPeriod(1);
  CreateThread(0, 0, i_winloop, 0, 0, 0);
  QueryPerformanceFrequency(&tnow);
  freq = 1.0 / tnow.QuadPart;
  do { _mm_pause(); dc = i_dc; _ReadWriteBarrier(); } while (!dc);
  hw = i_hw;
  _ReadWriteBarrier();

  if (g_setup() != 0)
    return 1;

  QueryPerformanceCounter(&tbase);
  for (;;) {
    QueryPerformanceCounter(&tnow);
    tnow.QuadPart -= tbase.QuadPart;
    t = freq * tnow.QuadPart;
    dt = freq * (tnow.QuadPart - tlast.QuadPart);
    tlast = tnow;

    dw = i_winsize;
    _ReadWriteBarrier();
    dh = dw >> 16;
    dw &= 0xFFFF;

    g_xwin = dw;
    g_ywin = dh;

    QueryPerformanceCounter(&tnow);
    if (g_frame(t, dt) != 0)
      break;
    QueryPerformanceCounter(&tloop);
    lt = freq * (tloop.QuadPart - tnow.QuadPart);

    StretchDIBits(dc, 0, 0, dw, dh, 0, 0, G_XRES, G_YRES, i_pixels, &bi,
                  DIB_RGB_COLORS, SRCCOPY);
    ValidateRect(hw, 0);
    i_dbgtime(hw, dt, lt, t);
  }
  return 0;
}
#endif
#endif
