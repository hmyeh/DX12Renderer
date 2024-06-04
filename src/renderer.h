#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <array>

#include "commandqueue.h"
#include "camera.h"
#include "texture.h"
#include "buffer.h"
#include "descriptorheap.h"
#include "pipeline.h"

// Forward declaration
class Scene;
class GUI;

class Renderer {
public:
    static constexpr unsigned int s_num_frames = 3;

private:
    // Making below a singleton
    static Microsoft::WRL::ComPtr<ID3D12Device2> DEVICE;

    // Use WARP adapter
    static bool use_warp;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain;
    DescriptorHeap m_rtv_descriptor_heap;
    unsigned int m_current_backbuffer_idx;
    std::array<RenderBuffer, s_num_frames> m_backbuffers;

    // Render textures
    std::array<RenderTargetTexture*, s_num_frames> m_render_textures;

    // Depth buffer
    DepthBuffer m_depth_buffer;
    // Descriptor heap for depth buffer.
    DescriptorHeap m_dsv_descriptor_heap;

    // Shader visible descriptor heap to be used everywhere
    FrameDescriptorHeap m_cbv_srv_descriptor_heap;

    // direct queue
    CommandQueue m_command_queue;

    DepthMapPipeline m_depthmap_pipeline;
    ScenePipeline m_scene_pipeline;
    ImagePipeline m_img_pipeline;

    // By default, enable V-Sync.
    // Can be toggled with the V key.
    bool m_vsync = true;
    bool m_tearing_supported = false;

    Scene* m_scene;
    Camera m_camera;
    TextureLibrary m_texture_library;

    HWND m_hWnd;
    DXGI_FORMAT m_render_target_format;
    DXGI_FORMAT m_depth_stencil_format;

    GUI* m_gui;

    uint32_t m_width, m_height;

public:
    Renderer(HWND hWnd, uint32_t width, uint32_t height, Scene* scene, GUI* gui, bool use_warp = false);
    ~Renderer();

    // Singleton device
    static Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice();

    // bind once for the shader visible descriptorheap 
    void Bind(Scene* scene); // be able to bind to new scene

    void Render();
    void LoadSizeDependentResources(unsigned int width, unsigned int height);

    void Resize(uint32_t width, uint32_t height);
    void ToggleVsync() { m_vsync = !m_vsync; }
};


