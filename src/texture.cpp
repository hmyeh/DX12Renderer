#include "texture.h"

#include <filesystem>

#include "utility.h"
#include "renderer.h"

// Texture

void Texture::Create(DirectX::TEX_DIMENSION dimensions, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags) {
    Destroy();

    CD3DX12_RESOURCE_DESC heap_resource_desc;
    switch (dimensions) {
    case DirectX::TEX_DIMENSION_TEXTURE2D:
        heap_resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, depth, mip_levels, 1, 0, flags);
        break;
    case DirectX::TEX_DIMENSION_TEXTURE3D:
        heap_resource_desc = CD3DX12_RESOURCE_DESC::Tex3D(format, width, height, depth, mip_levels, flags);
        break;
    case DirectX::TEX_DIMENSION_TEXTURE1D:
        heap_resource_desc = CD3DX12_RESOURCE_DESC::Tex1D(format, width, depth, mip_levels, flags);
        break;
    default:
        throw std::exception();
    }

    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    // Create a committed resource for the GPU resource in a default heap.
    ThrowIfFailed(device->CreateCommittedResource(
        &heap_props,
        D3D12_HEAP_FLAG_NONE,
        &heap_resource_desc,
        D3D12_RESOURCE_STATE_COPY_DEST, //hmm if no texture needs to be uploaded maybe different state...
        nullptr,
        IID_PPV_ARGS(&m_resource)));

    // Set appropriate initial resource state
    m_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
    //m_srv_handle = srv_handle;
    //CreateViews(); // turned off now since setting in texturelibrary after allocating descriptors
}

void Texture::CreateViews() {
    if (m_resource) {
        CD3DX12_RESOURCE_DESC desc(m_resource->GetDesc());
        Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
            device->CreateRenderTargetView(m_resource.Get(), nullptr, m_rtv_handle);

        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
            device->CreateDepthStencilView(m_resource.Get(), nullptr, m_dsv_handle);

        if ((desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
            device->CreateShaderResourceView(m_resource.Get(), nullptr, m_srv_handle);
    }
}

void Texture::Upload(CommandList& command_list) 
{
    // upload only works if it has an m_image
    if (m_image.GetImageCount() == 0) {
        throw std::exception("No Texture Images to upload");
    }

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    std::vector<D3D12_SUBRESOURCE_DATA> subresources(m_image.GetImageCount());
    const DirectX::Image* images = m_image.GetImages();
    for (int i = 0; i < m_image.GetImageCount(); ++i)
    {
        auto& subresource = subresources[i];
        subresource.RowPitch = images[i].rowPitch;
        subresource.SlicePitch = images[i].slicePitch;
        subresource.pData = images[i].pixels;
    }

    uint64_t required_size = GetRequiredIntermediateSize(m_resource.Get(), 0, subresources.size());

    // Upload data and update resource state
    command_list.UploadBufferData(required_size, m_resource.Get(), subresources.size(), subresources.data());
    TransitionResourceState(command_list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //command_list.ResourceBarrier(m_resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Texture::Bind(const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_desc_handle) 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    device->CopyDescriptorsSimple(1, cpu_desc_handle, m_srv_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Texture::Read(const std::wstring& file_name) 
{
    std::filesystem::path file_path(file_name);

    if (!std::filesystem::exists(file_path))
        throw std::exception("File not found");

    if (file_path.extension() == ".dds")
        ThrowIfFailed(DirectX::LoadFromDDSFile(file_name.c_str(), DirectX::DDS_FLAGS_FORCE_RGB, &m_metadata, m_image));
    else if (file_path.extension() == ".hdr")
        ThrowIfFailed(DirectX::LoadFromHDRFile(file_name.c_str(), &m_metadata, m_image));
    else if (file_path.extension() == ".tga")
        ThrowIfFailed(DirectX::LoadFromTGAFile(file_name.c_str(), &m_metadata, m_image));
    else
        ThrowIfFailed(DirectX::LoadFromWICFile(file_name.c_str(), DirectX::WIC_FLAGS_NONE, &m_metadata, m_image));

    Create(m_metadata.dimension, m_metadata.format, m_metadata.width, m_metadata.height, m_metadata.depth, m_metadata.mipLevels);

}


// set static samplers
std::unique_ptr<DescriptorHeap> TextureLibrary::s_sampler_heap = TextureLibrary::CreateSamplers();

// TODO: where to perform the resource barrier to pixel shader resource state? currently done here, so command queue is of type D3D12_COMMAND_LIST_TYPE_DIRECT
TextureLibrary::TextureLibrary() :
    m_command_queue(CommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT))
{
}

void TextureLibrary::AllocateDescriptors() {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    m_srv_heap = std::unique_ptr<DescriptorHeap>(new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_srv_texture_map.size() + m_rtv_textures.size() + m_dsv_textures.size()));
    m_rtv_heap = std::unique_ptr<DescriptorHeap>(new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_rtv_textures.size()));
    m_dsv_heap = std::unique_ptr<DescriptorHeap>(new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_dsv_textures.size()));

    // Set the Shader Resource View 
    unsigned int current_num_srv_tex = 0;
    for (const auto& [name, texture] : m_srv_texture_map) {
        texture->SetSrvHandle(m_srv_heap->GetCpuHandle(current_num_srv_tex));
        texture->CreateViews();
        ++current_num_srv_tex;
    }

    unsigned int current_num_rtv_tex = 0;
    for (auto& texture : m_rtv_textures) {
        texture->SetRtvHandle(m_rtv_heap->GetCpuHandle(current_num_rtv_tex));
        texture->SetSrvHandle(m_srv_heap->GetCpuHandle(current_num_srv_tex));
        texture->CreateViews();
        ++current_num_rtv_tex;
        ++current_num_srv_tex;
    }

    unsigned int current_num_dsv_tex = 0;
    for (auto& texture : m_rtv_textures) {
        texture->SetDsvHandle(m_dsv_heap->GetCpuHandle(current_num_dsv_tex));
        texture->SetSrvHandle(m_srv_heap->GetCpuHandle(current_num_srv_tex));
        texture->CreateViews();
        ++current_num_dsv_tex;
        ++current_num_srv_tex;
    }
}

Texture* TextureLibrary::CreateTexture(std::wstring file_name)
{
    auto result = m_srv_texture_map.find(file_name);
    if (result != m_srv_texture_map.end()) {
        return result->second.get();
    }
    
    std::unique_ptr<Texture> texture = std::make_unique<Texture>();
    texture->Read(file_name);
    m_srv_texture_map.insert(std::make_pair(file_name, std::move(texture)));

    return m_srv_texture_map[file_name].get();
}

Texture* TextureLibrary::CreateRenderTargetTexture(DXGI_FORMAT format, uint32_t width, uint32_t height) {
    std::unique_ptr<Texture> texture = std::make_unique<Texture>();
    texture->Create(DirectX::TEX_DIMENSION_TEXTURE2D, format, width, height, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    m_rtv_textures.push_back(std::move(texture));
    return m_rtv_textures.back().get();
}

Texture* TextureLibrary::CreateDepthTexture(DXGI_FORMAT format, uint32_t width, uint32_t height) {
    std::unique_ptr<Texture> texture = std::make_unique<Texture>();
    texture->Create(DirectX::TEX_DIMENSION_TEXTURE2D, format, width, height, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    m_dsv_textures.push_back(std::move(texture));
    return m_dsv_textures.back().get();
}

void TextureLibrary::Load() { // LOADING ALL TEXTURES AT ONCE
    // Allocate the descriptors before loading
    AllocateDescriptors();
    // load textures
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto command_list = m_command_queue.GetCommandList();

    for (const auto& [name, texture] : m_srv_texture_map) {
        texture->Upload(command_list);
    }

    auto fence_value = m_command_queue.ExecuteCommandList(command_list);
    m_command_queue.WaitForFenceValue(fence_value);
}


std::unique_ptr<DescriptorHeap> TextureLibrary::CreateSamplers()
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    std::unique_ptr<DescriptorHeap> sampler_heap(new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2, true));

    // Describe and create the wrapping sampler, which is used for 
    // sampling diffuse/normal maps.
    D3D12_SAMPLER_DESC wrap_sampler_desc = {};
    wrap_sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    wrap_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    wrap_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    wrap_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    wrap_sampler_desc.MinLOD = 0;
    wrap_sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    wrap_sampler_desc.MipLODBias = 0.0f;
    wrap_sampler_desc.MaxAnisotropy = 1;
    wrap_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    wrap_sampler_desc.BorderColor[0] = wrap_sampler_desc.BorderColor[1] = wrap_sampler_desc.BorderColor[2] = wrap_sampler_desc.BorderColor[3] = 0;
    device->CreateSampler(&wrap_sampler_desc, sampler_heap->GetCpuHandle());

    // Describe and create the point clamping sampler, which is 
    // used for the shadow map.
    D3D12_SAMPLER_DESC clamp_sampler_desc = {};
    clamp_sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    clamp_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    clamp_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    clamp_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    clamp_sampler_desc.MipLODBias = 0.0f;
    clamp_sampler_desc.MaxAnisotropy = 1;
    clamp_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    clamp_sampler_desc.BorderColor[0] = clamp_sampler_desc.BorderColor[1] = clamp_sampler_desc.BorderColor[2] = clamp_sampler_desc.BorderColor[3] = 0;
    clamp_sampler_desc.MinLOD = 0;
    clamp_sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    device->CreateSampler(&clamp_sampler_desc, sampler_heap->GetCpuHandle(1));

    return std::move(sampler_heap);
}
