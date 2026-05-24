#include "window.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

// Helper to convert HWND <-> void* without exposing windows.h
static HWND  asHWND(void* p)  { return (HWND)p; }
static void* fromHWND(HWND h) { return (void*)h; }

// ---------------------------------------------------------------------------
// Window class registration (once)
// ---------------------------------------------------------------------------
static const char* CLASS_NAME = "SimpleRendererWnd";
static bool classRegistered = false;

void Win32Window::registerWindowClass() {
    if (classRegistered) return;
    WNDCLASSA wc = {};
    wc.lpfnWndProc   = (WNDPROC)wndProc;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    RegisterClassA(&wc);
    classRegistered = true;
}

// ---------------------------------------------------------------------------
// DIB framebuffer — 32-bit BGRA, top-down
// ---------------------------------------------------------------------------
void Win32Window::createDIB() {
    HDC screenDC = GetDC(NULL);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;   // negative = top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    dibBitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS,
                                 (void**)&dibBits, NULL, 0);
    ReleaseDC(NULL, screenDC);

    // Create a memory DC and select the DIB into it
    HDC screen = GetDC(NULL);
    memDC = CreateCompatibleDC(screen);
    SelectObject((HDC)memDC, (HBITMAP)dibBitmap);
    ReleaseDC(NULL, screen);
}

// ---------------------------------------------------------------------------
// Window creation
// ---------------------------------------------------------------------------
Win32Window::Win32Window(int width, int height, const char* title)
    : w(width), h(height)
{
    registerWindowClass();
    createDIB();
    createWindow(title);
}

void Win32Window::createWindow(const char* title) {
    RECT rect = {0, 0, w, h};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    int winW = rect.right - rect.left;
    int winH = rect.bottom - rect.top;

    hwnd = fromHWND(CreateWindowExA(
        0, CLASS_NAME, title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        NULL, NULL, GetModuleHandleA(NULL), this));

    ShowWindow(asHWND(hwnd), SW_SHOW);
    UpdateWindow(asHWND(hwnd));
}

Win32Window::~Win32Window() {
    if (dibBitmap) DeleteObject((HBITMAP)dibBitmap);
    if (memDC)     DeleteDC((HDC)memDC);
    if (hwnd)      DestroyWindow(asHWND(hwnd));
}

// ---------------------------------------------------------------------------
// Copy float-RGB framebuffer → DIB, trigger redraw
// ---------------------------------------------------------------------------
void Win32Window::present(const Vec3* pixels) {
    if (!dibBits) return;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const Vec3& p = pixels[y * w + x];
            auto sat = [](float v) -> std::uint8_t {
                v = v < 0 ? 0 : (v > 1 ? 1 : v);
                return (std::uint8_t)(v * 255.0f + 0.5f);
            };
            int i = (y * w + x) * 4;
            dibBits[i + 0] = sat(p.z); // B
            dibBits[i + 1] = sat(p.y); // G
            dibBits[i + 2] = sat(p.x); // R
            dibBits[i + 3] = 255;      // A (unused)
        }
    }
    // Blit immediately and also invalidate for WM_PAINT
    blit();
}

void Win32Window::blit() {
    HDC winDC = GetDC(asHWND(hwnd));
    BitBlt(winDC, 0, 0, w, h, (HDC)memDC, 0, 0, SRCCOPY);
    ReleaseDC(asHWND(hwnd), winDC);
}

// ---------------------------------------------------------------------------
// Message loop
// ---------------------------------------------------------------------------
bool Win32Window::processMessages() {
    MSG msg;
    while (PeekMessageA(&msg, asHWND(hwnd), 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            shouldClose = true;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !shouldClose;
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
long long __stdcall Win32Window::wndProc(void* hwnd, unsigned msg,
                                          unsigned long long wParam, long long lParam)
{
    Win32Window* self = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTA* cs = (CREATESTRUCTA*)(LONG_PTR)lParam;
        self = (Win32Window*)cs->lpCreateParams;
        SetWindowLongPtrA((HWND)hwnd, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = (Win32Window*)GetWindowLongPtrA((HWND)hwnd, GWLP_USERDATA);
    }

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint((HWND)hwnd, &ps);
        if (self && self->memDC) {
            BitBlt(hdc,
                   ps.rcPaint.left, ps.rcPaint.top,
                   ps.rcPaint.right - ps.rcPaint.left,
                   ps.rcPaint.bottom - ps.rcPaint.top,
                   (HDC)self->memDC,
                   ps.rcPaint.left, ps.rcPaint.top,
                   SRCCOPY);
        }
        EndPaint((HWND)hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; // suppress flicker

    case WM_CLOSE:
        DestroyWindow((HWND)hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA((HWND)hwnd, msg, wParam, lParam);
}
