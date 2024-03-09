#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// STL Headers
#include <chrono>
#include <vector>

#include "utility.h"
#include "window.h"
#include "renderer.h"


class Application {

public:

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    // Set to true once the DX12 objects have been initialized.
    bool m_initialized;

    Application(HINSTANCE hInstance, const wchar_t* instance_name, uint32_t width, uint32_t height, bool use_warp = false, bool fullscreen = false);

    ~Application();

    void Show() { m_window->show(); }

    // Handle Windows messages
    LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


    // Game Loop
    void update();

    //void resize();

};
