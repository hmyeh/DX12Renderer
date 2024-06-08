#include "texture.h"

#include <filesystem>

#include "utility.h"
#include "renderer.h"


// ITexture

void ITexture::Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, const D3D12_CLEAR_VALUE& clear_value, bool use_clear_value, D3D12_RESOURCE_FLAGS flags)
{
    switch (dimension) {
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        m_resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, depth, mip_levels, 1, 0, flags);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        m_resource_desc = CD3DX12_RESOURCE_DESC::Tex3D(format, width, height, depth, mip_levels, flags);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        m_resource_desc = CD3DX12_RESOURCE_DESC::Tex1D(format, width, depth, mip_levels, flags);
        break;
    default:
        throw std::exception();
    }


    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    m_use_clear_value = use_clear_value;
    if (use_clear_value) {
        m_clear_value = clear_value;
        // Create a committed resource for the GPU resource in a default heap.
        ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &m_resource_desc, m_resource_state, &clear_value,
            IID_PPV_ARGS(&m_resource)));
    }
    else {
        // Create a committed resource for the GPU resource in a default heap.
        ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &m_resource_desc, m_resource_state, nullptr,
            IID_PPV_ARGS(&m_resource)));
    }
    m_flags = flags;
    
}

// Texture 

void Texture::Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags)
{
    // Set appropriate initial resource state
    m_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;

    // Create the resource
    ITexture::Create(dimension, format, width, height, depth, mip_levels, {}, false, flags);

    m_resource->SetName(L"Texture");

}

void Texture::Upload(CommandList& command_list)
{
    // upload only works if it has an m_image
    if (m_image.GetImageCount() == 0) {
        throw std::exception("Texture::Upload(): No Texture Images to upload");
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
}

void Texture::Read(const std::wstring& file_name)
{
    std::filesystem::path file_path(file_name);

    if (!std::filesystem::exists(file_path))
        throw std::exception("Texture::Read(): File not found");

    if (file_path.extension() == ".dds")
        ThrowIfFailed(DirectX::LoadFromDDSFile(file_name.c_str(), DirectX::DDS_FLAGS_FORCE_RGB, &m_metadata, m_image));
    else if (file_path.extension() == ".hdr")
        ThrowIfFailed(DirectX::LoadFromHDRFile(file_name.c_str(), &m_metadata, m_image));
    else if (file_path.extension() == ".tga")
        ThrowIfFailed(DirectX::LoadFromTGAFile(file_name.c_str(), &m_metadata, m_image));
    else
        ThrowIfFailed(DirectX::LoadFromWICFile(file_name.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, &m_metadata, m_image));

    Create(static_cast<D3D12_RESOURCE_DIMENSION>(m_metadata.dimension), m_metadata.format, m_metadata.width, m_metadata.height, m_metadata.depth, m_metadata.mipLevels, {});

}


// RenderTargetTexture

void RenderTargetTexture::Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags)
{
    // Set appropriate initial resource state
    m_resource_state = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // Optimized clear value
    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = format;
    clear_value.Color[0] = IRenderTarget::s_clear_value[0];
    clear_value.Color[1] = IRenderTarget::s_clear_value[1];
    clear_value.Color[2] = IRenderTarget::s_clear_value[2];
    clear_value.Color[3] = IRenderTarget::s_clear_value[3];

    // Create the resource
    ITexture::Create(dimension, format, width, height, depth, mip_levels, clear_value, true, flags | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    m_resource->SetName(L"Render Target Texture");
    
}

void RenderTargetTexture::ClearRenderTarget(CommandList& command_list)
{
    // Clearing the buffer means set state to rendertarget
    TransitionResourceState(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);
    command_list.ClearRenderTargetView(m_rtv_handle, IRenderTarget::s_clear_value);
}

// DepthMapTexture

void DepthMapTexture::Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags)
{
    // Set appropriate initial resource state
    m_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;

    // Optimized clear value
    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = IDepthStencilTarget::TypelessToDepthStencilFormat(format);
    clear_value.DepthStencil = IDepthStencilTarget::s_clear_value;

    // Create the resource
    ITexture::Create(dimension, format, width, height, depth, mip_levels, clear_value, true, flags | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    m_resource->SetName(L"Depth Stencil Texture");
}

void DepthMapTexture::ClearDepthStencil(CommandList& command_list)
{
    // Clearing the buffer means set state to rendertarget
    TransitionResourceState(command_list, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    command_list.ClearDepthStencilView(m_dsv_handle, IDepthStencilTarget::s_clear_value.Depth);
}


/// Texture Library

// set static samplers
std::unique_ptr<DescriptorHeap> TextureLibrary::s_sampler_heap = TextureLibrary::CreateSamplers();

TextureLibrary::TextureLibrary() :
    m_command_queue(CommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)),
    m_srv_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    m_rtv_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
    m_dsv_heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
    m_frame_descriptor_heap(nullptr)
{
}

void TextureLibrary::AllocateDescriptors() 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    m_srv_heap.Allocate(m_srv_texture_map.size() + m_rtv_textures.size() + m_dsv_textures.size());
    m_rtv_heap.Allocate(m_rtv_textures.size());
    m_dsv_heap.Allocate(m_dsv_textures.size());

    // Set the Shader Resource View 
    for (const auto& [name, texture] : m_srv_texture_map) {
        m_srv_heap.Bind(texture.get());
    }

    for (auto& texture : m_rtv_textures) {
        m_rtv_heap.Bind(texture.get());
        m_srv_heap.Bind(texture.get());
    }

    for (auto& texture : m_dsv_textures) {
        m_dsv_heap.Bind(texture.get());
        m_srv_heap.Bind(texture.get());
    }
}

void TextureLibrary::Bind(FrameDescriptorHeap* descriptor_heap)
{
    m_frame_descriptor_heap = descriptor_heap;
    for (unsigned int frame_idx = 0; frame_idx < Renderer::s_num_frames; ++frame_idx) {
        Bind(descriptor_heap, frame_idx);
    }
}

void TextureLibrary::Bind(FrameDescriptorHeap* descriptor_heap, unsigned int frame_idx) 
{
    if (descriptor_heap->GetHeapType() != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || !descriptor_heap->IsShaderVisible())
        throw std::exception("TextureLibrary::Bind(): Frame descriptor heap is not of correct type or visible to shader");

    // Binding all textures with ShaderResourceView
    for (auto& [name, texture] : m_srv_texture_map) {
        descriptor_heap->Bind(texture.get(), frame_idx);
    }

    for (auto& texture : m_rtv_textures) {
        descriptor_heap->Bind(texture.get(), frame_idx);
    }

    for (auto& texture : m_dsv_textures) {
        descriptor_heap->Bind(texture.get(), frame_idx);
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

RenderTargetTexture* TextureLibrary::CreateRenderTargetTexture(DXGI_FORMAT format, uint32_t width, uint32_t height)
{
    std::unique_ptr<RenderTargetTexture> texture = std::make_unique<RenderTargetTexture>();
    texture->Create(D3D12_RESOURCE_DIMENSION_TEXTURE2D, format, width, height, 1, 1);
    m_rtv_textures.push_back(std::move(texture));
    return m_rtv_textures.back().get();
}

DepthMapTexture* TextureLibrary::CreateDepthTexture(DXGI_FORMAT format, uint32_t width, uint32_t height)
{
    std::unique_ptr<DepthMapTexture> texture = std::make_unique<DepthMapTexture>();
    texture->Create(D3D12_RESOURCE_DIMENSION_TEXTURE2D, format, width, height, 1, 1);
    m_dsv_textures.push_back(std::move(texture));
    return m_dsv_textures.back().get();
}


void TextureLibrary::Load() 
{
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


void TextureLibrary::Reset() {
    m_command_queue.Flush();

    // Clear and reset everything
    m_srv_texture_map.clear();
    m_rtv_textures.clear();
    m_dsv_textures.clear();

    m_srv_heap.Reset();
    m_rtv_heap.Reset();
    m_dsv_heap.Reset();
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
