// gfx.h
// Written in 2016 by Mårten Hansson <hmrten@gmail.com>

#define G_XRES 640
#define G_YRES 480

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

typedef unsigned int u32_t;
typedef __declspec(align(16)) float v4_t[4];

extern void *g_fb;
extern u32_t g_dw;
extern u32_t g_dh;

int g_loop(double t, double dt);
int g_event(u32_t *ep);

void g_ods(const char *fmt, ...);
void g_delay(int ms);

void g_clear(v4_t col);

#ifdef GFX_C
void *g_fb;
u32_t g_dw;
u32_t g_dh;

void g_clear(v4_t col)
{
  u32_t r = (int)(col[0]*255.0f);
  u32_t g = (int)(col[1]*255.0f);
  u32_t b = (int)(col[2]*255.0f);
  u32_t a = (int)(col[3]*255.0f);
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  if (a > 255) a = 255;
  u32_t c = (a<<24) | (r<<16) | (g<<8) | b;
  __stosd((unsigned long *)g_fb, c, G_XRES*G_YRES);
}

#ifdef GFX_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
  static char str[256];
  static double ddt, ddt100, cdt, clt=1.0;
  static int fps, cfps;

  ++fps;
  ddt += dt;
  if (ddt >= 1.0) {
    cdt = dt*1000.0;
    clt = lt*1000.0;
    cfps = (int)(fps / ddt);
    fps = 0;
    ddt = 0.0;
  }

  ddt100 += dt;
  if (ddt >= 0.01) {
    int m = (int)(t/60.0);
    int s = (int)(t-m*60.0);
    int c = (int)(t*100.0)%100;
    sprintf(str, "%02d:%02d:%02d - %.2f ms / %4d fps (loop: %.2f ms / %4d fps)",
            m, s, c, cdt, cfps, clt, (int)(1000.0/clt));
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

  CreateThread(0, 0, i_winloop, 0, 0, 0);

  QueryPerformanceFrequency(&tnow);
  freq = 1.0 / tnow.QuadPart;

  do { _mm_pause(); _ReadWriteBarrier(); dc = i_dc; } while (!dc);

  _ReadWriteBarrier();
  hw = i_hw;

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
