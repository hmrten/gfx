// gfx.h
// Written in 2016 by Mårten Hansson <hmrten@gmail.com>

#define G_XRES 640
#define G_YRES 480

typedef unsigned int u32_t;

extern void *g_fb;

int g_loop(double t, double dt);

#ifdef GFX_C
void *g_fb;
#ifdef GFX_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static u32_t i_buf[G_XRES*G_YRES];
static HWND  i_hw;
static HDC   i_dc;
static DWORD i_winsize;
static BOOL  i_windone;

static LRESULT CALLBACK i_winproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
  switch (msg) {
  case WM_CLOSE:      PostQuitMessage(0); return 0;
  case WM_SIZE:       _ReadWriteBarrier(); i_winsize = (DWORD)lp; return 0;
  case WM_ERASEBKGND: return 1;
  case WM_KEYDOWN:    if (wp == VK_ESCAPE) PostQuitMessage(0); return 0;
  }
  return DefWindowProc(hw, msg, wp, lp);
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

  _ReadWriteBarrier();
  i_windone = 1;
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

  g_fb = i_buf;

  CreateThread(0, 0, i_winloop, 0, 0, 0);

  QueryPerformanceFrequency(&tnow);
  freq = 1.0 / tnow.QuadPart;

  do { _mm_pause(); _ReadWriteBarrier(); dc = i_dc; } while (!dc);

  _ReadWriteBarrier();
  hw = i_hw;

  QueryPerformanceCounter(&tbase);
  while (!i_windone) {
    _ReadWriteBarrier();

    QueryPerformanceCounter(&tnow);
    tnow.QuadPart -= tbase.QuadPart;
    t = freq * tnow.QuadPart;
    dt = freq * (tnow.QuadPart - tlast.QuadPart);
    tlast = tnow;

    DWORD dw = i_winsize;
    _ReadWriteBarrier();
    DWORD dh = dw >> 16;
    dw &= 0xFFFF;

    QueryPerformanceCounter(&tnow);
    g_loop(t, dt);
    QueryPerformanceCounter(&tloop);
    lt = freq * (tloop.QuadPart - tnow.QuadPart);

    StretchDIBits(dc, 0, 0, dw, dh, 0, 0, G_XRES, G_YRES, i_buf, &bi,
                  DIB_RGB_COLORS, SRCCOPY);
    ValidateRect(hw, 0);
    Sleep(1);
    i_dbgtime(hw, dt, lt, t);
  }
  ExitProcess(0);
}
#endif
#endif
