#pragma once

#include <Windows.h>

// STL Headers
#include <chrono>
#include <vector>

#include "window.h"
#include "renderer.h"
#include "scene.h"
#include "gui.h"

class Application {
private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<GUI> m_gui;

    // Set to true once the DX12 objects have been initialized.
    bool m_initialized;

public:
    Application(HINSTANCE hInstance, const wchar_t* instance_name, uint32_t width, uint32_t height, bool use_warp = false, bool fullscreen = false);
    ~Application();

    void Show() { m_window->Show(); }

    // Handle Windows messages
    LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Game Loop
    void Update();

};
