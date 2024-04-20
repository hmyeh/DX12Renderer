#include "pipeline.h"

#include "renderer.h"
#include "scene.h"

IPipeline::IPipeline() {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    // Check if root signature version 1.1 is available
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
    //feature_data.HighestVersion = ;
    bool supports_version_1_1 = FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data)));
    m_root_sig_feature_version = supports_version_1_1 ? D3D_ROOT_SIGNATURE_VERSION_1_1 : D3D_ROOT_SIGNATURE_VERSION_1_0;

    
}

// Scene Pipeline

void ScenePipeline::CreateRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // Create a root signature.
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
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, m_root_sig_feature_version, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
}

void ScenePipeline::CreatePipelineState() {
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

    // TODO: update the rtv formats/ use it to check when setting rendertargets
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    PipelineStateStream  pipelineStateStream;
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

void ScenePipeline::Render(unsigned int frame_idx, CommandList& command_list) {
    // setup and draw
    command_list.SetPipelineState(m_pipeline_state.Get());
    command_list.SetGraphicsRootSignature(m_root_signature.Get());

    command_list.SetViewport(*m_viewport);
    command_list.SetScissorRect(*m_scissor_rect);
    //Note: setdescriptorheaps moved out of here (to be set only once?)
    auto rtv_handle = m_rtv_renderbuffers[0]->GetDescHandle();
    auto dsv_handle = m_dsv_depthbuffer->GetDescHandle();
    command_list.SetRenderTargets(1, &rtv_handle, &dsv_handle);

    command_list.SetGraphicsRootDescriptorTable(1, m_scene->GetSceneConstantsHandle(frame_idx));
    command_list.SetGraphicsRootDescriptorTable(2, TextureLibrary::GetSamplerHeapGpuHandle());

    for (const auto& scene_item : m_scene->GetSceneItems()) {
        command_list.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list.SetVertexBuffer(scene_item.mesh.GetVertexBufferView());
        command_list.SetIndexBuffer(scene_item.mesh.GetIndexBufferView());

        command_list.SetGraphicsRootDescriptorTable(0, scene_item.gpu_handle[frame_idx]);

        // Draw
        command_list.DrawIndexedInstanced(CastToUint(scene_item.mesh.GetNumIndices()), 1);
    }
}
