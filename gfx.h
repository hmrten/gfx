// gfx.h
// Written in 2016 by Mårten Hansson <hmrten@gmail.com>

#define G_XRES 640
#define G_YRES 480

#define INLINE static __inline

#include <stddef.h>
#include <intrin.h>

enum {
  // events
  GE_QUIT=1, GE_KEYDOWN, GE_KEYUP, GE_KEYCHAR, GE_MOUSE,
  // keycodes ('0'..'9' and 'a'..'z' as lowercase ascii)
  GK_BACK=8, GK_TAB=9, GK_RET=13, GK_ESC=27, GK_SPACE=32, GK_CTRL=128,
  GK_SHIFT, GK_ALT, GK_UP, GK_DOWN, GK_LEFT, GK_RIGHT, GK_INS, GK_DEL,
  GK_HOME, GK_END, GK_PGUP, GK_PGDN, GK_F1, GK_F2, GK_F3, GK_F4, GK_F5,
  GK_F6, GK_F7, GK_F8, GK_F9, GK_F10, GK_F11, GK_F12, GK_M1, GK_M2, GK_M3,
  GK_M4, GK_M5
};

typedef signed   char       i8_t;
typedef unsigned char       u8_t;
typedef signed   short     i16_t;
typedef unsigned short     u16_t;
typedef signed   int       i32_t;
typedef unsigned int       u32_t;
typedef signed   long long i64_t;
typedef unsigned long long u64_t;

typedef ptrdiff_t idx_t;

typedef __declspec(align(16)) float v4_t[4];

extern void *g_fb;
extern u32_t g_dw;
extern u32_t g_dh;
extern u64_t g_perf[64][2];

#define g_assert(p) do { if (!(p)) *(volatile char *)0 = 1; } while(0)

#define g_perfbegin(id) u64_t perf_##id_t0 = __rdtsc()
#define g_perfend(id, n) (g_perf[id][0] = __rdtsc()-perf_##id_t0,\
                          g_perf[id][1] = (n))

int g_loop(double t, double dt);
int g_event(u32_t *ep);

void g_ods(const char *fmt, ...);
void g_delay(int ms);

void g_clear(v4_t col);
void g_rect(float x, float y, float w, float h, v4_t col);

INLINE void v4_fromint(v4_t v, u32_t i)
{
#ifdef USE_SSE
  __m128i m = _mm_set_epi32(0x8F8E8D03, 0x8B8A8900, 0x87868501, 0x83828102);
  __m128i p = _mm_shuffle_epi8(_mm_cvtsi32_si128(i), m);
  __m128 f = _mm_mul_ps(_mm_cvtepi32_ps(p), _mm_set1_ps(1.0f/255.0f));
  _mm_store_ps(v, f);
#else
  static const float inv255 = 1.0f / 255.0f;
  v[0] = inv255 * ((i>>16) & 0xFF);
  v[1] = inv255 * ((i>>8) & 0xFF);
  v[2] = inv255 * (i & 0xFF);
  v[3] = inv255 * (i>>24);
#endif
}

INLINE u32_t v4_toint(v4_t v)
{
#ifdef USE_SSE
  __m128i p = _mm_cvttps_epi32(_mm_mul_ps(_mm_load_ps(v), _mm_set1_ps(255.0f)));
  __m128i z = _mm_setzero_si128();
  p = _mm_shuffle_epi32(p, 0xC6);
  p = _mm_packs_epi32(p, z);
  p = _mm_packus_epi16(p, z);
  return _mm_cvtsi128_si32(p);
#else
  u32_t r = (int)(v[0]*255.0f);
  u32_t g = (int)(v[1]*255.0f);
  u32_t b = (int)(v[2]*255.0f);
  u32_t a = (int)(v[3]*255.0f);
  return (a<<24) | (r<<16) | (g<<8) | b;
#endif
}

INLINE void v4_mix(v4_t a, v4_t b, float t)
{
#ifdef USE_SSE
  __m128 aa = _mm_mul_ps(_mm_set1_ps(1.0f-t), _mm_load_ps(a));
  __m128 bb = _mm_mul_ps(_mm_set1_ps(t), _mm_load_ps(b));
  aa = _mm_add_ps(aa, bb);
  _mm_store_ps(a, aa);
#else
  a[0] = (1.0f-t)*a[0] + t*b[0];
  a[1] = (1.0f-t)*a[1] + t*b[1];
  a[2] = (1.0f-t)*a[2] + t*b[2];
  a[3] = (1.0f-t)*a[3] + t*b[3];
#endif
}

#ifdef GFX_C
void *g_fb;
u32_t g_dw;
u32_t g_dh;
u64_t g_perf[64][2];

static float g_xmaxf = (float)(G_XRES-1);
static float g_ymaxf = (float)(G_YRES-1);

void g_clear(v4_t col)
{
  __stosd((unsigned long *)g_fb, v4_toint(col), G_XRES*G_YRES);
}

void g_rect(float x, float y, float w, float h, v4_t col)
{
  g_perfbegin(0);

  float halfw = 0.5f*w;
  float halfh = 0.5f*h;

  idx_t ix0 = (idx_t)(x-halfw);
  idx_t ix1 = (idx_t)(x+halfw);
  idx_t iy0 = (idx_t)(y-halfh);
  idx_t iy1 = (idx_t)(y+halfh);

  if (ix0 < 0) ix0 = 0;
  if (ix1 > G_XRES) ix1 = G_XRES;
  if (iy0 < 0) iy0 = 0;
  if (iy1 > G_YRES) iy1 = G_YRES;

  idx_t iw = (idx_t)(ix1-ix0);
  idx_t ih = (idx_t)(iy1-iy0);

  if (iw <= 0 || ih <= 0) {
    g_perfend(0, 1);
    return;
  }

  g_assert(ix0 >= 0 && ix0 < G_XRES);
  g_assert(ix1 >= 0 && ix1 <= G_XRES);
  g_assert(iy0 >= 0 && iy0 < G_YRES);
  g_assert(iy1 >= 0 && iy1 <= G_YRES);

  u32_t *p = (u32_t *)g_fb + iy0*G_XRES + ix0;
  float t = col[3];
  for (idx_t y=0; y<ih; ++y, p+=G_XRES) {
    for (idx_t x=0; x<iw; ++x) {
      v4_t v;
      v4_fromint(v, p[x]);
      v4_mix(v, col, t);
      p[x] = v4_toint(v);
    }
  }

  g_perfend(0, iw*ih);
}

#ifdef GFX_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

static u32_t i_buf[G_XRES*G_YRES];
static HWND  i_hw;
static HDC   i_dc;
static DWORD i_winsize;
static DWORD i_qhead;
static u32_t i_qdata[256][2];
static DWORD i_qtail;

void g_ods(const char *fmt, ...)
{
  static char buf[1024];
  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  OutputDebugStringA(buf);
  va_end(args);
}

int g_event(u32_t *ep)
{
  DWORD head = i_qhead, tail = i_qtail;
  u32_t ev;
  _ReadWriteBarrier();
  if (head == tail) return 0;
  ev  = i_qdata[head][0];
  *ep = i_qdata[head][1];
  _ReadWriteBarrier();
  i_qhead = (head+1) & 255;
  return ev;
}

void g_delay(int ms)
{
  Sleep((DWORD)ms);
}

static int i_qadd(u32_t ev, u32_t ep)
{
  DWORD head = i_qhead, tail = i_qtail, ntail = (tail+1) & 255;
  _ReadWriteBarrier();
  if (ntail == head) return 0;
  i_qdata[tail][0] = ev;
  i_qdata[tail][1] = ep;
  _ReadWriteBarrier();
  i_qtail = ntail;
  return 1;
}

static const BYTE i_vkmap[256] = {
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
static LRESULT CALLBACK i_winproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
  u32_t ev=0, ep=0;
  switch (msg) {
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:  if ((lp&(1<<29)) && !(lp&(1<<31)) && wp == VK_F4) {
                         PostQuitMessage(0);
                         return 0;
                       }
  case WM_KEYUP:
  case WM_KEYDOWN:     if (!(lp&(1<<30)) == !!(lp&(1<<31))) return 0;
                       ev = (lp&(1<<31)) ? GE_KEYUP : GE_KEYDOWN;
                       if ((ep = i_vkmap[wp & 255])) goto qadd;
                       return 0;
  case WM_CHAR:        ev = GE_KEYCHAR; ep = (u32_t)wp; goto qadd;
  case WM_LBUTTONUP:   ep = GK_M1; goto mup;
  case WM_MBUTTONUP:   ep = GK_M2; goto mup;
  case WM_RBUTTONUP:   ep = GK_M3; goto mup;
  case WM_LBUTTONDOWN: ep = GK_M1; goto mdn;
  case WM_MBUTTONDOWN: ep = GK_M2; goto mdn;
  case WM_RBUTTONDOWN: ep = GK_M3; goto mdn;
  case WM_MOUSEWHEEL:  ep = ((int)wp>>16) / WHEEL_DELTA > 0 ? GK_M4 : GK_M5;
                       i_qadd(GE_KEYDOWN, ep);
                       i_qadd(GE_KEYUP, ep);
                       return 0;
  case WM_MOUSEMOVE:   ev = GE_MOUSE;
                       ep = (u32_t)lp;
                       goto qadd;
  case WM_SIZE:       _ReadWriteBarrier(); i_winsize = (DWORD)lp; return 0;
  case WM_ERASEBKGND: return 1;
  case WM_CLOSE:      PostQuitMessage(0); return 0;
  }
  return DefWindowProc(hw, msg, wp, lp);

mup:  ev = GE_KEYUP; goto qadd;
mdn:  ev = GE_KEYDOWN;
qadd: i_qadd(ev, ep);
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
  i_qadd(GE_QUIT, 0);
  return 0;
}

static void i_dbgtime(HWND hw, double dt, double lt, double t)
{
  static char unit = 'M';
  static char str[256];
  static double ddt, ddt100, cdt, clt=1.0, ccy;
  static u32_t ccyd;
  static int fps, cfps;

  ++fps;
  ddt += dt;
  if (ddt >= 1.0) {
    cdt = dt*1000.0;
    clt = lt*1000.0;
    cfps = (int)(fps / ddt);
    u64_t cy = g_perf[0][0];
    ccyd = (u32_t)(cy / g_perf[0][1]);

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

__declspec(noreturn) void WinMainCRTStartup(void)
{
  static BITMAPINFO bi = { sizeof(BITMAPINFOHEADER), G_XRES, -G_YRES, 1, 32 };
  LARGE_INTEGER tbase, tnow, tlast = { 0 }, tloop;
  double freq, t, dt, lt;
  HWND  hw;
  HDC   dc;
  u32_t dw, dh;

  g_fb = i_buf;

  timeBeginPeriod(1);

  CreateThread(0, 0, i_winloop, 0, 0, 0);

  QueryPerformanceFrequency(&tnow);
  freq = 1.0 / tnow.QuadPart;

  do { _mm_pause(); dc = i_dc; _ReadWriteBarrier(); } while (!dc);

  hw = i_hw;
  _ReadWriteBarrier();

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

    g_dw = dw;
    g_dh = dh;

    QueryPerformanceCounter(&tnow);
    if (!g_loop(t, dt))
      break;
    QueryPerformanceCounter(&tloop);
    lt = freq * (tloop.QuadPart - tnow.QuadPart);

    StretchDIBits(dc, 0, 0, dw, dh, 0, 0, G_XRES, G_YRES, i_buf, &bi,
                  DIB_RGB_COLORS, SRCCOPY);
    ValidateRect(hw, 0);
    i_dbgtime(hw, dt, lt, t);
  }
  ExitProcess(0);
}
#endif
#endif
