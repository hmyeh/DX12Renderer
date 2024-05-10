#include "descriptorheap.h"

#include "renderer.h"
#include "utility.h"
#include "texture.h"
#include "buffer.h"
#include "gui.h"

// IDescriptorHeap

IDescriptorHeap::IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, bool shader_visible) : 
    m_heap_type(heap_type), m_num_descriptors(0), m_shader_visible(shader_visible) 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    m_descriptor_size = device->GetDescriptorHandleIncrementSize(m_heap_type);
}

IDescriptorHeap::IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, unsigned int num_descriptors, bool shader_visible) : 
    m_heap_type(heap_type), m_num_descriptors(num_descriptors), m_shader_visible(shader_visible) 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    m_descriptor_size = device->GetDescriptorHandleIncrementSize(m_heap_type);

    Allocate(num_descriptors);
}

void IDescriptorHeap::Allocate(unsigned int num_descriptors) 
{
    m_num_descriptors = num_descriptors;

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = m_num_descriptors;//m_num_frame_descriptors * Renderer::s_num_frames; // 1 srv diffuse texture (will be changed every model/mesh) + 1 cbv * framecount
    heap_desc.Flags = m_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.Type = m_heap_type;
    ThrowIfFailed(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_descriptor_heap)));
}

// DescriptorHeap

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::FindResourceHandle(GpuResource* resource) 
{
    auto result = m_resource_descriptor_map.find(resource);
    if (result == m_resource_descriptor_map.end())
        throw std::exception("Resource not contained in descriptor heap");

    return GetGpuHandle(result->second);
}

// do it the other way around
unsigned int DescriptorHeap::Bind(Texture* texture) 
{
    if (m_resource_descriptor_map.size() == m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    unsigned int bind_idx = CastToUint(m_resource_descriptor_map.size());
    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);

    switch (GetHeapType()) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        texture->BindShaderResourceView(handle);
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        texture->BindRenderTargetView(handle);
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
        texture->BindDepthStencilView(handle);
        break;
    default:
        throw std::exception("weird heap type for binding...");
    }

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptor_map.insert({ dynamic_cast<GpuResource*>(texture), bind_idx });
    //texture->Bind(GetCpuHandle(m_num_bound_descriptors));
    //++m_num_bound_descriptors;
    return bind_idx;
}


unsigned int DescriptorHeap::Bind(RenderBuffer* render_buffer) 
{
    unsigned int bind_idx = CastToUint(m_resource_descriptor_map.size());

    if (bind_idx == m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");
    
    //D3D12_CPU_DESCRIPTOR_HANDLE testhandle = m_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);
    

    render_buffer->BindRenderTargetView(handle);

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptor_map.insert({ dynamic_cast<GpuResource*>(render_buffer), bind_idx });

    //++m_num_bound_descriptors;
    return bind_idx;
}

unsigned int DescriptorHeap::Bind(DepthBuffer* depth_buffer) 
{
    unsigned int bind_idx = CastToUint(m_resource_descriptor_map.size());

    if (bind_idx == m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);
    depth_buffer->BindDepthStencilView(GetCpuHandle(bind_idx));

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptor_map.insert({ dynamic_cast<GpuResource*>(depth_buffer), bind_idx });

    //++m_num_bound_descriptors;
    return bind_idx;
}


/// FrameDescriptorHeap

FrameDescriptorHeap::FrameDescriptorHeap(unsigned int num_frames) : 
    m_num_frames(num_frames), m_resource_descriptor_maps(num_frames), IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true)
{

}

FrameDescriptorHeap::FrameDescriptorHeap(unsigned int num_frames, unsigned int num_descriptors) : 
    m_num_frames(num_frames), m_num_frame_descriptors(num_descriptors), m_resource_descriptor_maps(num_frames), IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true)
{
    Allocate(num_descriptors);
}

void FrameDescriptorHeap::Allocate(unsigned int num_descriptors) {
    m_num_frame_descriptors = num_descriptors;
    IDescriptorHeap::Allocate(num_descriptors * m_num_frames);
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameDescriptorHeap::FindResourceHandle(GpuResource* resource, unsigned int frame_idx)
{
    auto result = m_resource_descriptor_maps[frame_idx].find(resource);
    if (result == m_resource_descriptor_maps[frame_idx].end())
        throw std::exception("Resource not contained in descriptor heap");

    unsigned int resource_idx = result->second;
    return GetGpuHandle(result->second);

}

unsigned int FrameDescriptorHeap::Bind(Texture* texture, unsigned int frame_idx) {
    if (m_resource_descriptor_maps[frame_idx].size() == m_num_frame_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    unsigned int bind_idx = CastToUint(m_resource_descriptor_maps[frame_idx].size()) + frame_idx * m_num_frame_descriptors;
    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);
    texture->BindShaderResourceView(handle);

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptor_maps[frame_idx].insert({ dynamic_cast<GpuResource*>(texture), bind_idx });

    return bind_idx;
}

unsigned int FrameDescriptorHeap::Bind(UploadBuffer* constant_buffer, unsigned int frame_idx)
{
    if (m_resource_descriptor_maps[frame_idx].size() == m_num_frame_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    unsigned int bind_idx = CastToUint(m_resource_descriptor_maps[frame_idx].size()) + frame_idx * m_num_frame_descriptors;
    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);
    constant_buffer->BindConstantBufferView(handle);

    // Cache in map that constant_buffer is bound at bind_idx
    m_resource_descriptor_maps[frame_idx].insert({ dynamic_cast<GpuResource*>(constant_buffer), bind_idx });

    return bind_idx;
}

unsigned int FrameDescriptorHeap::Bind(ImguiResource* resource, unsigned int frame_idx)
{
    if (m_resource_descriptor_maps[frame_idx].size() == m_num_frame_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    unsigned int bind_idx = CastToUint(m_resource_descriptor_maps[frame_idx].size()) + frame_idx * m_num_frame_descriptors;

    // Cache in map that resource is bound at bind_idx
    m_resource_descriptor_maps[frame_idx].insert({ dynamic_cast<GpuResource*>(resource), bind_idx });

    return bind_idx;
}
