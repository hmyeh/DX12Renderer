#include "renderer.h"

#include "scene.h"
#include <iostream>


/// Renderer

bool Renderer::use_warp = false;
Microsoft::WRL::ComPtr<ID3D12Device2> Renderer::DEVICE = nullptr;

Renderer::Renderer(HWND hWnd, uint32_t width, uint32_t height, bool use_warp) :
    m_scissor_rect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
    m_viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))),
    m_camera(width, height),
    m_command_queue(D3D12_COMMAND_LIST_TYPE_DIRECT)
{
    m_tearing_supported = directx::CheckTearingSupport();

    Microsoft::WRL::ComPtr<ID3D12Device2> device = GetDevice();

    m_swap_chain = directx::CreateSwapChain(hWnd, m_command_queue.GetD12CommandQueue(), width, height, s_num_frames);

    m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

    // Create the backbuffers (renderbuffers)
    m_rtv_descriptor_heap = std::unique_ptr<DescriptorHeap>(new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, s_num_frames));
    CreateBackbuffers();

    // Create the descriptor heap for the depth-stencil view.
    m_dsv_descriptor_heap = std::unique_ptr<DescriptorHeap>(new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1));
    m_depth_buffer.Create(width, height, m_dsv_descriptor_heap->GetCpuHandle());

    // create pipeline
    m_pipeline.Init();
    m_pipeline.SetViewport(&m_viewport);
    m_pipeline.SetScissorRect(&m_scissor_rect);
    m_pipeline.SetCamera(&m_camera);
}

Renderer::~Renderer() {
    m_command_queue.Flush();
}

// Singleton device
Microsoft::WRL::ComPtr<ID3D12Device2> Renderer::GetDevice()
{
    if (DEVICE == nullptr) {
        //SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // Check for DirectX Math library support.
        if (!DirectX::XMVerifyCPUSupport())
        {
            MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
            throw std::exception();
        }

        directx::EnableDebugLayer();

        Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4 = directx::GetAdapter(use_warp);
        DEVICE = directx::CreateDevice(dxgiAdapter4);
    }
    return DEVICE;
}


void Renderer::CreateBackbuffers()
{
    for (int i = 0; i < s_num_frames; ++i)
    {
        m_backbuffers[i].Create(m_swap_chain, i, m_rtv_descriptor_heap->GetCpuHandle(i));
    }
}

void Renderer::Bind(Scene* scene) 
{
    m_scene = scene;
    // Create the shader visible CBV/SRV/UAV descriptor heap
    m_cbv_srv_descriptor_heap = std::unique_ptr<DescriptorHeap>(new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_scene->GetNumFrameDescriptors() * s_num_frames, true)); // TODO: add number of descriptors for pipelines
    m_scene->Bind(m_cbv_srv_descriptor_heap.get());

    // set pipeline scene
    m_pipeline.SetScene(m_scene);
}

void Renderer::Render()
{
    // Scene needs to be bound before rendering
    if (!m_scene)
        throw std::exception("No scene is bound to the renderer");

    RenderBuffer& backbuffer = m_backbuffers[m_current_backbuffer_idx];
    CommandList command_list = m_command_queue.GetCommandList();

    // Set descriptor heaps once here
    command_list.SetDescriptorHeaps({m_cbv_srv_descriptor_heap.get(), TextureLibrary::GetSamplerHeap()});

    // Set render target
    m_pipeline.SetRenderTargets({ &backbuffer }, &m_depth_buffer);

    // Clear the render target.
    m_pipeline.Clear(command_list);

    // Update the scene cbv
    m_scene->Update(m_current_backbuffer_idx, m_camera);

    // Run Pipeline render
    m_pipeline.Render(m_current_backbuffer_idx, command_list);

    // Present
    {
        backbuffer.TransitionResourceState(command_list, D3D12_RESOURCE_STATE_PRESENT);

        uint64_t fence_value = m_command_queue.ExecuteCommandList(command_list);
        backbuffer.SetFenceValue(fence_value);

        UINT sync_interval = m_vsync ? 1 : 0;
        UINT present_flags = m_tearing_supported && !m_vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(m_swap_chain->Present(sync_interval, present_flags));

        m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

        m_command_queue.WaitForFenceValue(backbuffer.GetFenceValue());
    }
}


void Renderer::Resize(uint32_t width, uint32_t height) {
    // Flush the GPU queue to make sure the swap chain's back buffers
     // are not being referenced by an in-flight command list.
    m_command_queue.Flush();

    uint64_t current_fence_value = m_backbuffers[m_current_backbuffer_idx].GetFenceValue();
    for (int i = 0; i < s_num_frames; ++i)
    {
        // Any references to the back buffers must be released
        // before the swap chain can be resized.
        m_backbuffers[i].Destroy();
        m_backbuffers[i].SetFenceValue(current_fence_value);
    }

    // Resize backbuffers (renderbuffers)
    DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
    ThrowIfFailed(m_swap_chain->GetDesc(&swapchain_desc));
    ThrowIfFailed(m_swap_chain->ResizeBuffers(s_num_frames, width, height,
        swapchain_desc.BufferDesc.Format, swapchain_desc.Flags));

    m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();
    CreateBackbuffers();

    // Resize depth buffer
    m_depth_buffer.Resize(width, height);

    // TODO: Should also resize "other render targets" and m_viewport
}
