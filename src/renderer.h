#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "utility.h"
#include "dx12_api.h"
#include "commandqueue.h"
#include "scene.h"
#include "camera.h"


// PSO
// TODO: pipeline factory?
class Pipeline {
public:
    // Root signature
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;

    // Pipeline state object.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state;

    D3D12_VIEWPORT* m_viewport;
    D3D12_RECT* m_scissor_rect;

    D3D12_CPU_DESCRIPTOR_HANDLE* m_render_target_view; 
    D3D12_CPU_DESCRIPTOR_HANDLE* m_depth_stencil_view;

    Pipeline(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor_rect);


    // Set rendertarget(s)
    //void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* render_target_view, D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_view) {

    //}

    void ClearRenderTarget(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list, ID3D12Resource* render_buffer, D3D12_CPU_DESCRIPTOR_HANDLE* render_target_view, D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_view);

    // bind the meshes from the scene here, since the actual renderer doesnt need to see it actually
    void RenderSceneToTarget(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list, const Scene& scene, const Camera& camera, D3D12_CPU_DESCRIPTOR_HANDLE* render_target_view, D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_view);
};

class Renderer {
public:
    static const unsigned int s_num_frames = 3;

    // Making below a singleton
    static Microsoft::WRL::ComPtr<ID3D12Device2> DEVICE;

    // Use WARP adapter
    static bool use_warp;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_backbuffers[s_num_frames];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap;
    UINT m_rtv_descriptor_size;
    UINT m_current_backbuffer_idx;

    // Depth buffer.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depth_buffer;
    // Descriptor heap for depth buffer.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap;

    // direct queue
    CommandQueue m_command_queue;

    

    uint64_t m_frame_fence_values[s_num_frames] = {};


    std::unique_ptr<Pipeline> m_pipeline;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;


    
    // By default, enable V-Sync.
    // Can be toggled with the V key.
    bool m_vsync = true;
    bool m_tearing_supported = false;

    Camera m_camera;
    Scene m_scene;

    Renderer(HWND hWnd, uint32_t width, uint32_t height, bool use_warp = false);

    ~Renderer();

    // Singleton device
    static Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice();


    void UpdateRenderTargetViews();

    void render();


    void resize(uint32_t width, uint32_t height);

    void resizeDepthBuffer(uint32_t width, uint32_t height);

    void toggleVsync() {
        m_vsync = !m_vsync;
    }
};


