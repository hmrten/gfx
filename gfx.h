// gfx.h
// Written in 2016 by Mårten Hansson <hmrten@gmail.com>

#define G_XRES 640
#define G_YRES 480

#define INLINE static __inline

#include <stddef.h>
#include <intrin.h>

enum {
  // events
  GE_INIT=1, GE_QUIT, GE_KEYDOWN, GE_KEYUP, GE_KEYCHAR, GE_MOUSE,
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

#define g_min(a, b) ((a) < (b) ? (a) : (b))
#define g_max(a, b) ((a) > (b) ? (a) : (b))

int g_loop(double t, double dt);
int g_event(u32_t *ep);

void g_ods(const char *fmt, ...);
void g_delay(int ms);

void g_clear(v4_t col);
void g_rect(float x, float y, float w, float h, v4_t col);
void g_line(float x0, float y0, float x1, float y1, v4_t col);

void g_texquad(float x, float y, v4_t m, v4_t col);

INLINE float v4_min(v4_t v)
{
  __m128 x = _mm_set_ss(v[0]);
  x = _mm_min_ss(x, _mm_set_ss(v[1]));
  x = _mm_min_ss(x, _mm_set_ss(v[2]));
  x = _mm_min_ss(x, _mm_set_ss(v[3]));
  return _mm_cvtss_f32(x);
}

INLINE float v4_max(v4_t v)
{
  __m128 x = _mm_set_ss(v[0]);
  x = _mm_max_ss(x, _mm_set_ss(v[1]));
  x = _mm_max_ss(x, _mm_set_ss(v[2]));
  x = _mm_max_ss(x, _mm_set_ss(v[3]));
  return _mm_cvtss_f32(x);
}

INLINE __m128 ps_fromint(u32_t i)
{
  __m128i sm = _mm_set_epi32(0x80808003, 0x80808000, 0x80808001, 0x80808002);
  __m128i pi = _mm_shuffle_epi8(_mm_cvtsi32_si128(i), sm);
  return _mm_mul_ps(_mm_cvtepi32_ps(pi), _mm_set1_ps(1.0f/255.0f));
}

INLINE u32_t ps_toint(__m128 ps)
{
  __m128i pi = _mm_cvttps_epi32(_mm_mul_ps(ps, _mm_set1_ps(255.0f)));
  __m128i zi = _mm_setzero_si128();
  pi = _mm_shuffle_epi32(pi, 0xC6);
  pi = _mm_packus_epi16(_mm_packs_epi32(pi, zi), zi);
  return _mm_cvtsi128_si32(pi);
}

#define ps_madd(x, y, z) _mm_add_ps(_mm_mul_ps(x, y), z)
#define ps_scale1(p, s) _mm_mul_ps(p, _mm_set1_ps(s))

#ifdef GFX_C
void *g_fb;
u32_t g_dw;
u32_t g_dh;
u64_t g_perf[64][2];

static float g_xmaxf = (float)(G_XRES-1);
static float g_ymaxf = (float)(G_YRES-1);

typedef struct {
  void *data;
  u32_t w, h;
  int   sx, sy, sw, sh;
} texdef_t;

#define G_TEXCOUNT 32

static texdef_t g_texlist[G_TEXCOUNT];
static u32_t    g_texcur;

void g_clear(v4_t col)
{
  __m128 ps = _mm_load_ps(col);
  __stosd((unsigned long *)g_fb, ps_toint(ps), G_XRES*G_YRES);
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

  float t = col[3];
  __m128 m = _mm_set1_ps(1.0f-t);
  __m128 a = ps_scale1(_mm_load_ps(col), t);
  u32_t *p = (u32_t *)g_fb + iy0*G_XRES + ix0;
  for (idx_t y=0; y<ih; ++y, p+=G_XRES) {
    for (idx_t x=0; x<iw; ++x)
      p[x] = ps_toint(ps_madd(ps_fromint(p[x]), m, a));
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

void g_line(float x0, float y0, float x1, float y1, v4_t col)
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
  idx_t e0 = dx > 0 ? 1 : -1;
  idx_t e1 = e0;
  idx_t step0 = dy > 0 ? G_XRES : -G_XRES;
  idx_t step1 = 0;
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
  u32_t *p = (u32_t *)g_fb + iy0*G_XRES + ix0;
  do {
    *p = ps_toint(ps_madd(ps_fromint(*p), m, a));
    d += j;
    if (d >= i)
      d -= i, p += step0;
    else
      p += step1;
  } while (n--);
}

static const float g_texw = 128.0f;
static const float g_texh = 128.0f;

void g_texquad(float x, float y, v4_t m, v4_t col)
{
  g_perfbegin(0);

  m[0] *= g_texw;
  m[2] *= g_texw;
  m[1] *= g_texh;
  m[3] *= g_texh;

  x -= 0.5f*(m[0]+m[2]);
  y -= 0.5f*(m[1]+m[3]);

  v4_t bx = { x, x+m[0], x+m[0]+m[2], x+m[2] };
  v4_t by = { y, y+m[1], y+m[1]+m[3], y+m[3] };
  int xmin = (int)v4_min(bx);
  int xmax = (int)v4_max(bx);
  int ymin = (int)v4_min(by);
  int ymax = (int)v4_max(by);
  if (xmin < 0) xmin = 0;
  if (ymin < 0) ymin = 0;
  if (xmax > G_XRES-1) xmax = G_XRES-1;
  if (ymax > G_YRES-1) ymax = G_YRES-1;

  float d01 = 1.0f / sqrtf(m[0]*m[0] + m[1]*m[1]);
  float d23 = 1.0f / sqrtf(m[2]*m[2] + m[3]*m[3]);
  d01 *= d01;
  d23 *= d23;
  float dm[] = { d01*m[0], d01*m[1], d23*m[2], d23*m[3] };
  float sx = (float)xmin - x;
  float sy = (float)ymin - y;
  float ur = sx*dm[0] + sy*dm[1];
  float vr = sx*dm[2] + sy*dm[3];

  texdef_t *td = g_texlist + g_texcur;
  u32_t tw = td->w;
  u32_t *texptr = (u32_t *)td->data + td->sy*tw + td->sx;
  float sw = (float)td->sw;
  float sh = (float)td->sh;

  idx_t w = xmax-xmin+1;
  idx_t h = ymax-ymin+1;
  u32_t *prow = (u32_t *)g_fb + ymin*G_XRES + xmin;
  u32_t *plast = (u32_t *)g_fb + ymax*G_XRES;
  while (prow <= plast) {
    float u = ur;
    float v = vr;
    for (idx_t i=0; i<w; ++i) {
      if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
        u32_t tx = (int)(u*sw);
        u32_t ty = (int)(v*sh);
        u32_t tex = texptr[ty*tw+tx];
        prow[i] = tex;
      } else {
        prow[i] = 0xCCCCCC;
      }
      u += dm[0];
      v += dm[2];
    }
    ur += dm[1];
    vr += dm[3];
    prow += G_XRES;
  }

  g_perfend(0, w*h);
}

#ifdef GFX_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

static u32_t  i_buf[G_XRES*G_YRES];
static HWND   i_hw;
static HDC    i_dc;
static DWORD  i_winsize;
static DWORD  i_qhead;
static u32_t  i_qdata[256][2];
static DWORD  i_qtail;
static HANDLE i_heap;

void g_texload(u32_t slot, const char *name)
{
  u8_t hdr[18], type, depth;
  u16_t w, h;
  DWORD nread, toread;

  g_assert(slot < G_TEXCOUNT);

  HANDLE f = CreateFile(name, GENERIC_READ, 0, 0, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 0);
  g_assert(f != INVALID_HANDLE_VALUE);

  ReadFile(f, hdr, sizeof hdr, &nread, 0);
  g_assert(nread == sizeof hdr);

  type = hdr[0x02];
  w = (hdr[0x0D] << 8) | hdr[0x0C];
  h = (hdr[0x0F] << 8) | hdr[0x0E];
  depth = hdr[0x10];

  g_assert(type == 2 && depth == 32);

  texdef_t *td = g_texlist + g_texcur;
  toread = w*h*4;
  td->data = HeapAlloc(i_heap, 0, toread);
  td->w = w;
  td->h = h;
  td->sx = 0;
  td->sy = 0;
  td->sw = w;
  td->sh = h;
  ReadFile(f, td->data, toread, &nread, 0);
  g_assert(nread == toread);

  CloseHandle(f);
}

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

  i_heap = GetProcessHeap();

  g_fb = i_buf;

  timeBeginPeriod(1);

  CreateThread(0, 0, i_winloop, 0, 0, 0);

  QueryPerformanceFrequency(&tnow);
  freq = 1.0 / tnow.QuadPart;

  do { _mm_pause(); dc = i_dc; _ReadWriteBarrier(); } while (!dc);

  hw = i_hw;
  _ReadWriteBarrier();

  i_qadd(GE_INIT, 0);

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
