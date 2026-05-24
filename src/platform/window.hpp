#pragma once
#include "math.hpp"
#include <vector>
#include <cstdint>

class Win32Window {
public:
    Win32Window(int width, int height, const char* title);
    ~Win32Window();

    bool processMessages();           // returns false when window closes
    void present(const Vec3* pixels); // copy float RGB → DIB and repaint

    int  width()  const { return w; }
    int  height() const { return h; }
    bool closed() const { return shouldClose; }

private:
    int  w, h;
    bool shouldClose = false;
    void* hwnd = nullptr;          // HWND  (opaque to avoid windows.h in header)

    void*        dibBitmap = nullptr; // HBITMAP
    std::uint8_t* dibBits  = nullptr; // pointer to BGRA pixel memory
    void*        memDC     = nullptr; // HDC  compatible DC holding the DIB

    void createDIB();
    void blit();
    void registerWindowClass();
    void createWindow(const char* title);

    static long long __stdcall wndProc(void* hwnd, unsigned msg, unsigned long long wParam, long long lParam);
};
