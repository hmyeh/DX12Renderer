#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <array>

#include "utility.h"
#include "dx12_api.h"
#include "commandqueue.h"
#include "camera.h"
#include "texture.h"
#include "buffer.h"
#include "descriptorheap.h"
#include "pipeline.h"

// Forward declaration
class Scene;


class Renderer {
public:
    static constexpr unsigned int s_num_frames = 3;

private:
    // Making below a singleton
    static Microsoft::WRL::ComPtr<ID3D12Device2> DEVICE;

    // Use WARP adapter
    static bool use_warp;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain;
    std::unique_ptr<DescriptorHeap> m_rtv_descriptor_heap;
    unsigned int m_current_backbuffer_idx;
    std::array<RenderBuffer, s_num_frames> m_backbuffers;

    // Depth buffer.
    DepthBuffer m_depth_buffer;
    // Descriptor heap for depth buffer.
    std::unique_ptr<DescriptorHeap> m_dsv_descriptor_heap;

    // Shader visible descriptor heap to be used everywhere
    std::unique_ptr<DescriptorHeap> m_cbv_srv_descriptor_heap;

    // direct queue
    CommandQueue m_command_queue;

    ScenePipeline m_pipeline;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;
    
    // By default, enable V-Sync.
    // Can be toggled with the V key.
    bool m_vsync = true;
    bool m_tearing_supported = false;

    Scene* m_scene;
    Camera m_camera;
    TextureLibrary m_texture_library;

public:
    Renderer(HWND hWnd, uint32_t width, uint32_t height, bool use_warp = false);
    ~Renderer();

    // Singleton device
    static Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice();

    // bind once for the shader visible descriptorheap 
    void Bind(Scene* scene);

    void Render();

    void Resize(uint32_t width, uint32_t height);

    void ToggleVsync() { m_vsync = !m_vsync; }

private:
    void CreateBackbuffers();
};


