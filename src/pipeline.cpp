#include "pipeline.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "renderer.h"
#include "scene.h"
#include "utility.h"
#include "camera.h"
#include "texture.h"
#include "buffer.h"
#include "descriptorheap.h"
#include "gui.h"

#if _DEBUG
const std::wstring IPipeline::s_compiled_shader_path = L"x64/Debug/";
#else
const std::wstring IPipeline::s_compiled_shader_path = L"x64/Release/";
#endif

IPipeline::IPipeline():
    m_scissor_rect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
    m_viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, 0.0f, 0.0f))
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    // Check if root signature version 1.1 is available
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
    //feature_data.HighestVersion = ;
    bool supports_version_1_1 = FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data)));
    m_root_sig_feature_version = supports_version_1_1 ? D3D_ROOT_SIGNATURE_VERSION_1_1 : D3D_ROOT_SIGNATURE_VERSION_1_0;
}

void IPipeline::Render(unsigned int frame_idx, CommandList& command_list)
{
    // setup and draw
    command_list.SetPipelineState(m_pipeline_state.Get());
    command_list.SetGraphicsRootSignature(m_root_signature.Get());

    command_list.SetViewport(m_viewport);
    command_list.SetScissorRect(m_scissor_rect);

    command_list.SetRenderTargets(m_rtv_rendertargets, m_dsv_rendertarget); 
}


// Depthmap Pipeline

void DepthMapPipeline::CreateRootSignature() 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // Create a root signature.
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1]; // Perfomance TIP: Order from most frequent to least frequent.
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 1 frequently changed constant buffer.

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

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

void DepthMapPipeline::CreatePipelineState() 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // Load the vertex shader.
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    std::wstring vs = s_compiled_shader_path + L"depthmap_vs.cso";
    ThrowIfFailed(D3DReadFileToBlob(vs.c_str(), &vertexShaderBlob));

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXTURE_COORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // TODO: update the rtv formats/ use it to check when setting rendertargets
    D3D12_RT_FORMAT_ARRAY rtvFormats = { DXGI_FORMAT_UNKNOWN };
    rtvFormats.NumRenderTargets = 0;
    //rtvFormats.RTFormats[0] = DXGI_FORMAT_UNKNOWN;

    PipelineStateStream pipelineStateStream;
    pipelineStateStream.pRootSignature = m_root_signature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(0, 0);
    pipelineStateStream.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
    pipelineStateStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state
    pipelineStateStream.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blend state.
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipeline_state)));
}

void DepthMapPipeline::Clear(CommandList& command_list)
{
    // Set the rendertarget to the lights depthmap
    SetRenderTargets({}, m_scene->GetDirectionalLightDepthMap()); // TODO: with pointlights need to do this for each light
    IPipeline::Clear(command_list);
}

void DepthMapPipeline::Render(unsigned int frame_idx, CommandList& command_list)
{

    // Set the rendertarget to the lights depthmap
    //SetRenderTargets({}, m_scene->GetDirectionalLightDepthMap()); // TODO: with pointlights need to do this for each light

    IPipeline::Render(frame_idx, command_list);

    command_list.SetGraphicsRootDescriptorTable(1, m_scene->GetSceneConstantsHandle(frame_idx));

    for (const auto& scene_item : m_scene->GetSceneItems()) {
        command_list.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list.SetVertexBuffer(scene_item.mesh.GetVertexBufferView());
        command_list.SetIndexBuffer(scene_item.mesh.GetIndexBufferView());

        DirectX::XMMATRIX model = scene_item.GetModelMatrix();
        model = DirectX::XMMatrixTranspose(model);
        command_list.SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX) / 4, &model, 0);

        // Draw
        command_list.DrawIndexedInstanced(CastToUint(scene_item.mesh.GetNumIndices()), 1);
    }
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
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 ranges[4]; // Perfomance TIP: Order from most frequent to least frequent.
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 2 frequently changed diffuse + normal textures - using registers t1 and t2.
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 1 frequently changed constant buffer.
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);                                                // 1 infrequently changed shadow texture - starting in register t0.
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);                                            // 2 static samplers.

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[6];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstants(sizeof(MaterialParams) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[4].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

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
    std::wstring vs = s_compiled_shader_path + L"vertex.cso";
    ThrowIfFailed(D3DReadFileToBlob(vs.c_str(), &vertexShaderBlob));

    // Load the pixel shader.
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
    std::wstring ps = s_compiled_shader_path + L"pixel.cso";
    ThrowIfFailed(D3DReadFileToBlob(ps.c_str(), &pixelShaderBlob));

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXTURE_COORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // TODO: update the rtv formats/ use it to check when setting rendertargets
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    PipelineStateStream pipelineStateStream;
    pipelineStateStream.pRootSignature = m_root_signature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
    pipelineStateStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state
    pipelineStateStream.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blend state.
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipeline_state)));
}

void ScenePipeline::Render(unsigned int frame_idx, CommandList& command_list) {
    IPipeline::Render(frame_idx, command_list);

    // Prepare to set in shader_pixel_resource state
    m_scene->GetDirectionalLightDepthMap()->UseShaderResource(command_list);

    command_list.SetGraphicsRootDescriptorTable(3, m_scene->GetSceneConstantsHandle(frame_idx));
    command_list.SetGraphicsRootDescriptorTable(4, m_scene->GetDirectionalLightHandle(frame_idx));
    command_list.SetGraphicsRootDescriptorTable(5, TextureLibrary::GetSamplerHeapGpuHandle());

    for (const auto& scene_item : m_scene->GetSceneItems()) {
        command_list.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list.SetVertexBuffer(scene_item.mesh.GetVertexBufferView());
        command_list.SetIndexBuffer(scene_item.mesh.GetIndexBufferView());
        
        DirectX::XMMATRIX model = scene_item.GetModelMatrix();
        model = DirectX::XMMatrixTranspose(model);
        command_list.SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX) / 4, &model, 0);
        command_list.SetGraphicsRoot32BitConstants(1, sizeof(MaterialParams) / 4, scene_item.mesh.GetMaterial(), 0);
        command_list.SetGraphicsRootDescriptorTable(2, scene_item.resource_handles[frame_idx][0]);// grabbing diffuse tex hardcoded

        // Draw
        command_list.DrawIndexedInstanced(CastToUint(scene_item.mesh.GetNumIndices()), 1);
    }
}


// ImagePipeline

ImagePipeline::ImagePipeline() : m_textures(Renderer::s_num_frames), m_texture_handles(Renderer::s_num_frames), IPipeline() {
}

void ImagePipeline::CreateRootSignature() 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // Create a root signature.
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 ranges[2]; // Perfomance TIP: Order from most frequent to least frequent.
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);// , D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 2 frequently changed diffuse + normal textures - using registers t1 and t2.
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);                                            // 2 static samplers.

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[3];
    rootParameters[0].InitAsConstants(ImageOptions::GetNum32BitValues(), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

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

void ImagePipeline::CreatePipelineState() 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // Load the shaders
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    std::wstring vs = s_compiled_shader_path + L"image_vs.cso";
    ThrowIfFailed(D3DReadFileToBlob(vs.c_str(), &vertexShaderBlob));

    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
    std::wstring ps = s_compiled_shader_path + L"image_ps.cso";
    ThrowIfFailed(D3DReadFileToBlob(ps.c_str(), &pixelShaderBlob));

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXTURE_COORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // TODO: update the rtv formats/ use it to check when setting rendertargets
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Disable depth test
    CD3DX12_DEPTH_STENCIL_DESC depth_stencil_state(D3D12_DEFAULT);
    depth_stencil_state.DepthEnable = false;

    PipelineStateStream  pipelineStateStream;
    pipelineStateStream.pRootSignature = m_root_signature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
    pipelineStateStream.DepthStencilState = depth_stencil_state;
    pipelineStateStream.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blend state.
    pipelineStateStream.DSVFormat = DXGI_FORMAT_UNKNOWN;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipeline_state)));
}

void ImagePipeline::SetInputTexture(int frame_idx, RenderTargetTexture* texture)
{
    m_textures[frame_idx] = texture;
}


void ImagePipeline::Render(unsigned int frame_idx, CommandList& command_list) 
{
    IPipeline::Render(frame_idx, command_list);

    // Prepare to set in shader_pixel_resource state
    m_textures[frame_idx]->UseShaderResource(command_list);

    // Set the appropriate descriptors for the shader
    command_list.SetGraphicsRoot32BitConstants(0, ImageOptions::GetNum32BitValues(), m_img_options, 0);
    command_list.SetGraphicsRootDescriptorTable(1, m_textures[frame_idx]->GetShaderGPUHandle(frame_idx));
    command_list.SetGraphicsRootDescriptorTable(2, TextureLibrary::GetSamplerHeapGpuHandle());

    // Set the quad buffers
    command_list.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list.SetVertexBuffer(m_quad.GetVertexBufferView());
    command_list.SetIndexBuffer(m_quad.GetIndexBufferView());

    //// Draw
    command_list.DrawIndexedInstanced(CastToUint(m_quad.GetNumIndices()), 1);
}
