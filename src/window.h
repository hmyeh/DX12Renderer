#pragma once

#include <Windows.h>

//// In order to define a function called CreateWindow, the Windows macro needs to
//// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

#include <functional>

class Window {
private:
    // Window handle.
    HWND m_hWnd;
    // Window rectangle (used to toggle fullscreen state).
    RECT m_window_rect;
    
    // By default, use windowed mode.
    // Can be toggled with the Alt+Enter or F11
    bool m_fullscreen;

    uint32_t m_width;
    uint32_t m_height;

    // Bound member function to attach to Window
    std::function<LRESULT CALLBACK(HWND, UINT, WPARAM, LPARAM)> m_WndProc;

    static const wchar_t* s_window_class_name;

public:
    Window(HINSTANCE hInstance, const wchar_t* instance_name, uint32_t width, uint32_t height, std::function<LRESULT CALLBACK(HWND, UINT, WPARAM, LPARAM)> m_WndProc, bool fullscreen = false);

    // Window callback function.
    //https://stackoverflow.com/questions/18161680/how-do-i-stdbind-a-non-static-class-member-to-a-win32-callback-function-wnd?rq=3
    static LRESULT CALLBACK WndProcHelper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void RegisterWindowClass(HINSTANCE hInst);
    HWND CreateWindow(HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height);

    HWND GetWindowHandle() const { return m_hWnd; }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    // Don't allow 0 size swap chain back buffers.
    void SetWidth(uint32_t width) { m_width = std::max(1u, width); }
    void SetHeight(uint32_t height) { m_height = std::max(1u, height); }

    bool IsFullscreen() const { return m_fullscreen; }

    //
    void Show();
    void ToggleFullscreen();
    bool Resize();
};
