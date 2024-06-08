#include "window.h"

#include <cassert>


const wchar_t* Window::s_window_class_name = L"renderer";

Window::Window(HINSTANCE hInstance, const wchar_t* instance_name, uint32_t width, uint32_t height, std::function<LRESULT CALLBACK(HWND, UINT, WPARAM, LPARAM)> WndProc, bool fullscreen) : 
    m_width(width), m_height(height), m_WndProc(WndProc), m_fullscreen(fullscreen)
{
    RegisterWindowClass(hInstance);
    m_hWnd = CreateWindow(hInstance, instance_name, m_width, m_height);

    // Initialize the global window rect variable.
    ::GetWindowRect(m_hWnd, &m_window_rect);
}


// Windows Helper Functions

LRESULT CALLBACK Window::WndProcHelper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, 0));
    if (window)
        return window->m_WndProc(hwnd, msg, wParam, lParam);
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void Window::RegisterWindowClass(HINSTANCE hInst)
{
    // If a previous window was already initialized, dont reregister
    //if (m_initialized) TODO: fix this
    //    return;

    // Register a window class for creating our render window with.
    WNDCLASSEXW windowClass = {};

    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &WndProcHelper;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = sizeof(Window*);
    windowClass.hInstance = hInst;
    windowClass.hIcon = ::LoadIcon(hInst, NULL); //  MAKEINTRESOURCE(APPLICATION_ICON));
    windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = Window::s_window_class_name;
    windowClass.hIconSm = ::LoadIcon(hInst, NULL); //  MAKEINTRESOURCE(APPLICATION_ICON));

    static HRESULT hr = ::RegisterClassExW(&windowClass);
    assert(SUCCEEDED(hr));
}

HWND Window::CreateWindow(HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    HWND hWnd = ::CreateWindowExW(
        NULL,
        Window::s_window_class_name,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        windowWidth,
        windowHeight,
        NULL,
        NULL,
        hInst,
        nullptr
    );
    // Store instance pointer
    SetWindowLongPtrW(hWnd, 0, reinterpret_cast<LONG_PTR>(this));

    assert(hWnd && "Failed to create window");

    return hWnd;
}


/// Window Functions

void Window::Show()
{
    ::ShowWindow(m_hWnd, SW_SHOW);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}


void Window::ToggleFullscreen()
{
    m_fullscreen = !m_fullscreen;

    if (m_fullscreen) // Switching to fullscreen.
    {
        // Store the current window dimensions so they can be restored 
        // when switching out of fullscreen state.
        ::GetWindowRect(m_hWnd, &m_window_rect);

        // Set the window style to a borderless window so the client area fills
        // the entire screen.
        UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

        ::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

        // Query the name of the nearest display device for the window.
        // This is required to set the fullscreen dimensions of the window
        // when using a multi-monitor setup.
        HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(hMonitor, &monitorInfo);

        ::SetWindowPos(m_hWnd, HWND_TOPMOST,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_MAXIMIZE);
    }
    else
    {
        // Restore all the window decorators.
        ::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        ::SetWindowPos(m_hWnd, HWND_NOTOPMOST,
            m_window_rect.left,
            m_window_rect.top,
            m_window_rect.right - m_window_rect.left,
            m_window_rect.bottom - m_window_rect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_NORMAL);
    }
}

bool Window::Resize()
{
    RECT clientRect = {};
    ::GetClientRect(m_hWnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    if (m_width != width || m_height != height) 
    {
        SetWidth(width);
        SetHeight(height);
        return true;
    }
    return false;
}
