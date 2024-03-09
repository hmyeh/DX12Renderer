#include "application.h"


// D3D12 extension library.
#include <d3dx12.h>
#include <DirectXMath.h>

#include "dx12_api.h"

#include <functional>


Application::Application(HINSTANCE hInstance, const wchar_t* instance_name, uint32_t width, uint32_t height, bool use_warp, bool fullscreen)
{
    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window 
    // to achieve 100% scaling while still allowing non-client window content to 
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        throw std::exception();
    }

    directx::EnableDebugLayer();

    //m_tearing_supported = directx::CheckTearingSupport();

    // Assign member update function to Window
    auto update_fn = std::bind(&Application::WndProc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    m_window = std::unique_ptr<Window>(new Window(hInstance, instance_name, width, height, update_fn, fullscreen));


    m_renderer = std::unique_ptr<Renderer>(new Renderer(m_window->getWindowHandle(), width, height, use_warp));

    m_initialized = true;
}

Application::~Application() {
}


LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!m_initialized) 
        return ::DefWindowProcW(hwnd, message, wParam, lParam);

    switch (message)
    {
    case WM_PAINT:
        update();
        m_renderer->render();
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        switch (wParam)
        {
        case 'V':
            m_renderer->toggleVsync();
            break;
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
        case VK_RETURN:
            if (alt)
            {
        case VK_F11:
            m_window->toggleFullscreen();
            }
            break;
        }
    }
    break;
    // The default window procedure will play a system notification sound 
    // when pressing the Alt+Enter keyboard combination if this message is 
    // not handled.
    case WM_SYSCHAR:
        break;
    case WM_SIZE:
    {
        if (m_window->resize()) {
            uint32_t width = m_window->getWidth();
            uint32_t height = m_window->getHeight();

            m_renderer->resize(width, height);
            m_renderer->resizeDepthBuffer(width, height);
        }

        //resize();
    }
    break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }
}


void Application::update()
{
    static uint64_t frameCounter = 0;
    static double elapsedSeconds = 0.0;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();

    frameCounter++;
    auto t1 = clock.now();
    auto deltaTime = t1 - t0;
    t0 = t1;

    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0)
    {
        wchar_t buffer[500];
        auto fps = frameCounter / elapsedSeconds;
        swprintf_s(buffer, 500, L"FPS: %f\n", fps);
        OutputDebugString(buffer);

        frameCounter = 0;
        elapsedSeconds = 0.0;
    }
}
