#include "renderer.h"

#include "scene.h"
#include "utility.h"
#include "dx12_api.h"
#include "gui.h"

/// Renderer

bool Renderer::use_warp = false;
Microsoft::WRL::ComPtr<ID3D12Device2> Renderer::DEVICE = nullptr;

Renderer::Renderer(HWND hWnd, uint32_t width, uint32_t height, Scene* scene, GUI* gui,  bool use_warp) :
    m_hWnd(hWnd),
    m_render_target_format(DXGI_FORMAT_R8G8B8A8_UNORM),
    m_depth_stencil_format(DXGI_FORMAT_D32_FLOAT),
    m_camera(width, height),
    m_gui(gui),
    m_width(width),
    m_height(height),
    m_command_queue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_rtv_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, s_num_frames),
    m_dsv_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1u),
    m_cbv_srv_descriptor_heap(s_num_frames, gui->GetNumResources()),
    m_initialized(false)
{
    m_tearing_supported = directx::CheckTearingSupport();

    Microsoft::WRL::ComPtr<ID3D12Device2> device = GetDevice();
    
    // Create Swapchain
    m_swap_chain = directx::CreateSwapChain(hWnd, m_command_queue.GetD12CommandQueue(), width, height, s_num_frames);
    m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

    // Create the size dependent resources
    LoadSizeDependentResources(width, height);

    // Create the descriptors for the render target textures
    m_texture_library.AllocateDescriptors();
    // Bind the scene and other cbv srv resources to the shader visible heap
    Bind(scene);
    
    // Initialize Pipelines
    SetupPipelines();

    // Bind gui data
    BindGuiData();

    m_initialized = true;
}

Renderer::~Renderer() {
    m_command_queue.Flush();
}

void Renderer::BindGuiData()
{
    // Set Gui pointers
    m_gui->SetImageOptions(m_img_pipeline.GetOptions());
}

void Renderer::SetupPipelines()
{
    // Initialize Pipelines
    m_depthmap_pipeline.Init(&m_cbv_srv_descriptor_heap, m_scene);
    m_scene_pipeline.Init(&m_cbv_srv_descriptor_heap, m_width, m_height, m_scene, &m_camera);
    m_img_pipeline.Init(&m_command_queue, &m_cbv_srv_descriptor_heap, m_width, m_height);

    // Set the input texture for the image pipeline
    for (int i = 0; i < s_num_frames; ++i)
        m_img_pipeline.SetInputTexture(i, m_render_textures[i]);
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


void Renderer::Bind(Scene* scene) 
{
    m_scene = scene;

    // Create the shader visible CBV/SRV/UAV descriptor heap
    m_cbv_srv_descriptor_heap.Reset();
    m_cbv_srv_descriptor_heap.Allocate(m_scene->GetNumFrameDescriptors() + m_texture_library.GetNumTextures());

    // Bind imgui resource
    m_gui->Bind(&m_cbv_srv_descriptor_heap, m_render_target_format);

    // Bind the render target textures
    m_texture_library.Bind(&m_cbv_srv_descriptor_heap);

    // Bind the scene descriptors
    m_scene->Bind(&m_cbv_srv_descriptor_heap);
}



void Renderer::Render()
{
    // Scene needs to be bound before rendering
    if (!m_scene)
        throw std::exception("Renderer::Render(): No scene is bound to the renderer");

    RenderBuffer& backbuffer = m_backbuffers[m_current_backbuffer_idx];
    CommandList command_list = m_command_queue.GetCommandList();

    // Set descriptor heaps once here
    command_list.SetDescriptorHeaps({&m_cbv_srv_descriptor_heap, TextureLibrary::GetSamplerHeap()});

    //// Update the scene cbv
    m_scene->Update(m_current_backbuffer_idx, m_camera);

    // Run depth map pipeline
    m_depthmap_pipeline.Clear(command_list);
    m_depthmap_pipeline.Render(m_current_backbuffer_idx, command_list);

    ////// Run Scene pipeline
    m_scene_pipeline.SetRenderTargets({ m_render_textures[m_current_backbuffer_idx]}, &m_depth_buffer);
    m_scene_pipeline.Clear(command_list);
    m_scene_pipeline.Render(m_current_backbuffer_idx, command_list);

    //// Run image pipeline
    m_img_pipeline.SetRenderTargets({ &backbuffer }, nullptr);
    m_img_pipeline.Clear(command_list);
    m_img_pipeline.Render(m_current_backbuffer_idx, command_list);

    //// Draw Imgui 
    m_gui->Render(command_list);

    // Present
    {
        backbuffer.Present(command_list);

        m_command_queue.ExecuteCommandList(command_list);

        UINT sync_interval = m_vsync ? 1 : 0;
        UINT present_flags = m_tearing_supported && !m_vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(m_swap_chain->Present(sync_interval, present_flags));


        const uint64_t fence_value = m_backbuffers[m_current_backbuffer_idx].GetFenceValue();
        m_command_queue.Signal(fence_value);
        // update frameidx
        m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

        // Wait
        m_command_queue.WaitForFenceValue(m_backbuffers[m_current_backbuffer_idx].GetFenceValue());
        m_backbuffers[m_current_backbuffer_idx].SetFenceValue(fence_value + 1);
    }
}


void Renderer::LoadSizeDependentResources(unsigned int width, unsigned int height)
{
    // Create the backbuffers (renderbuffers)
    for (int i = 0; i < s_num_frames; ++i)
    {
        m_backbuffers[i].Create(m_swap_chain, i);
        if (!m_initialized)
            m_rtv_descriptor_heap.Bind(&m_backbuffers[i]);
    }

    // Create depth stencil buffer
    if (m_initialized) {
        m_depth_buffer.Resize(width, height);
    }
    else {
        m_depth_buffer.Create(m_depth_stencil_format, width, height);
        m_dsv_descriptor_heap.Bind(&m_depth_buffer);
    }
    

    for (int i = 0; i < s_num_frames; ++i)
    {
        if (m_render_textures[i])
            m_render_textures[i]->Resize(width, height);
        else
            m_render_textures[i] = m_texture_library.CreateRenderTargetTexture(m_render_target_format, width, height);
    }

    // Resize pipelines
    m_scene_pipeline.Resize(width, height);
    m_img_pipeline.Resize(width, height);

    // Camera aspect ratio resize
    m_camera.Resize(width, height);
}


void Renderer::Resize(uint32_t width, uint32_t height) 
{
    // Flush the GPU queue to make sure the swap chain's back buffers
        // are not being referenced by an in-flight command list.
    //m_command_queue.Flush();
    uint64_t fence_value = m_backbuffers[m_current_backbuffer_idx].GetFenceValue();
    m_command_queue.Signal(fence_value);
    m_command_queue.WaitForFenceValue(fence_value);

    for (int i = 0; i < s_num_frames; ++i)
    {
        // Any references to the back buffers must be released
        // before the swap chain can be resized.
        m_backbuffers[i].Destroy();
        m_backbuffers[i].SetFenceValue(fence_value + 1);
    }

    // Resize backbuffers (renderbuffers)
    DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
    ThrowIfFailed(m_swap_chain->GetDesc(&swapchain_desc));
    ThrowIfFailed(m_swap_chain->ResizeBuffers(s_num_frames, width, height, swapchain_desc.BufferDesc.Format, swapchain_desc.Flags));

    // Update backbuffer index
    m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();
    m_width = width;
    m_height = height;
    // Resize size dependent resources
    LoadSizeDependentResources(width, height);
}
