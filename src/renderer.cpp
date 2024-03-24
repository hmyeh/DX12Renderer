#include "renderer.h"

#include "scene.h"
#include <iostream>

/// Pipeline

Pipeline::Pipeline(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor_rect) :
    m_viewport(viewport), m_scissor_rect(scissor_rect)
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // Load the vertex shader.
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"E:/Rendering/x64/Debug/vertex.cso", &vertexShaderBlob));

    // Load the pixel shader.
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"E:/Rendering/x64/Debug/pixel.cso", &pixelShaderBlob));

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXTURE_COORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;// |
        //D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 ranges[3]; // Perfomance TIP: Order from most frequent to least frequent.
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 2 frequently changed diffuse + normal textures - using registers t1 and t2.
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 1 frequently changed constant buffer.
    //ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);                                                // 1 infrequently changed shadow texture - starting in register t0.
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);                                            // 2 static samplers.

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[3];
    //rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4 * 3, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
    //rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    // Serialize the root signature.
    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.pRootSignature = m_root_signature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipeline_state)));
}

//void Pipeline::ClearRenderTarget(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list, ID3D12Resource* render_buffer, D3D12_CPU_DESCRIPTOR_HANDLE* render_target_view, D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_view) {
//
//    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
//        render_buffer,
//        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET); // TODO: previous state is only PRESENT IN CASE OF BACKBUFFER NOT FOR OTHER TEXTURES/RESOURCES!!!
//
//    command_list->ResourceBarrier(1, &barrier);
//
//    FLOAT clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };
//
//
//    float depth = 1.0f;
//    command_list->ClearRenderTargetView(*render_target_view, clear_color, 0, nullptr);
//    command_list->ClearDepthStencilView(*depth_stencil_view, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
//
//}

// bind the meshes from the scene here, since the actual renderer doesnt need to see it actually
void Pipeline::RenderSceneToTarget(unsigned int frame_idx, CommandList& command_list, Scene& scene, const Camera& camera, D3D12_CPU_DESCRIPTOR_HANDLE* render_target_view, D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_view) {
    // setup and draw
    command_list.SetPipelineState(m_pipeline_state.Get());
    command_list.SetGraphicsRootSignature(m_root_signature.Get());

    ID3D12DescriptorHeap* heaps[] = { scene.GetCbvSrvHeap(), scene.GetTextureSamplers() };
    command_list.SetDescriptorHeaps(_countof(heaps), heaps);

    command_list.SetViewport(*m_viewport);
    command_list.SetScissorRect(*m_scissor_rect);

    command_list.SetRenderTargets(1, render_target_view, depth_stencil_view);

    // Update the scene cbv
    scene.Update(frame_idx, camera);

    command_list.SetGraphicsRootDescriptorTable(1, scene.GetSceneConstantsHandle(frame_idx));
    command_list.SetGraphicsRootDescriptorTable(2, scene.GetTextureSamplersHandle());

    for (const Mesh& mesh : scene.GetItems()) {
        // Bind descriptors of the mesh (texture)
        scene.BindSceneItem(frame_idx, mesh);

        command_list.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list.SetVertexBuffer(mesh.GetVertexBufferView());
        command_list.SetIndexBuffer(mesh.GetIndexBufferView());

        command_list.SetGraphicsRootDescriptorTable(0, scene.GetDiffuseTextureHandle(frame_idx));

        // Draw
        command_list.DrawIndexedInstanced(mesh.GetNumIndices(), 1);
    }
}


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

    m_rtv_descriptor_heap = directx::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, s_num_frames);
    m_rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews();

    // Create the descriptor heap for the depth-stencil view.
    m_dsv_descriptor_heap = directx::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    ResizeDepthBuffer(width, height);

    // create pipeline
    m_pipeline = std::unique_ptr<Pipeline>(new Pipeline(&m_viewport, &m_scissor_rect));
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


void Renderer::UpdateRenderTargetViews()
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = GetDevice();
    auto rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < s_num_frames; ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> backbuffer;
        ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

        device->CreateRenderTargetView(backbuffer.Get(), nullptr, rtv_handle);

        m_backbuffers[i] = backbuffer;

        rtv_handle.Offset(rtv_descriptor_size);
    }
}

void Renderer::Render(Scene& scene)
{

    auto backbuffer = m_backbuffers[m_current_backbuffer_idx];
    CommandList command_list = m_command_queue.GetCommandList();

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
        m_current_backbuffer_idx, m_rtv_descriptor_size);
    auto dsv = m_dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

    // Clear the render target.
    {
        command_list.ResourceBarrier(backbuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        

        float depth = 1.0f;
        command_list.ClearRenderTargetView(rtv, clear_color);
        command_list.ClearDepthStencilView(dsv, depth);
    }

    // Run Pipeline render
    m_pipeline->RenderSceneToTarget(m_current_backbuffer_idx, command_list, scene, m_camera, &rtv, &dsv);


    // Present
    {
        command_list.ResourceBarrier(backbuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        m_frame_fence_values[m_current_backbuffer_idx] = m_command_queue.ExecuteCommandList(command_list);

        UINT sync_interval = m_vsync ? 1 : 0;
        UINT present_flags = m_tearing_supported && !m_vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(m_swap_chain->Present(sync_interval, present_flags));

        m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

        m_command_queue.WaitForFenceValue(m_frame_fence_values[m_current_backbuffer_idx]);
    }
}


void Renderer::Resize(uint32_t width, uint32_t height) {
    // Flush the GPU queue to make sure the swap chain's back buffers
     // are not being referenced by an in-flight command list.
    m_command_queue.Flush();

    for (int i = 0; i < s_num_frames; ++i)
    {
        // Any references to the back buffers must be released
        // before the swap chain can be resized.
        m_backbuffers[i].Reset();
        m_frame_fence_values[i] = m_frame_fence_values[m_current_backbuffer_idx];
    }

    DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
    ThrowIfFailed(m_swap_chain->GetDesc(&swapchain_desc));
    ThrowIfFailed(m_swap_chain->ResizeBuffers(s_num_frames, width, height,
        swapchain_desc.BufferDesc.Format, swapchain_desc.Flags));

    m_current_backbuffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

    UpdateRenderTargetViews();
}

void Renderer::ResizeDepthBuffer(uint32_t width, uint32_t height) {
    m_command_queue.Flush();//

    // Resize screen dependent resources.
    // Create a depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    Microsoft::WRL::ComPtr<ID3D12Device2> device = GetDevice();
    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ThrowIfFailed(device->CreateCommittedResource(
        &heap_props,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_depth_buffer)
    ));

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(m_depth_buffer.Get(), &dsv,
        m_dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
}

