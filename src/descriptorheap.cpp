#include "descriptorheap.h"

#include <algorithm>

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
    heap_desc.NumDescriptors = m_num_descriptors;//m_num_frame_descriptors * Renderer::s_num_frames;
    heap_desc.Flags = m_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.Type = m_heap_type;
    ThrowIfFailed(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_descriptor_heap)));
}

// DescriptorHeap


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::FindResourceHandle(GpuResource* resource) 
{
    auto result = std::find(m_resource_descriptors.begin(), m_resource_descriptors.end(), resource);
    if (result == m_resource_descriptors.end())
        throw std::exception("Resource not contained in descriptor heap");
    unsigned int bind_idx = result - m_resource_descriptors.begin();

    return GetGpuHandle(bind_idx);
}

// do it the other way around
unsigned int DescriptorHeap::Bind(Texture* texture) 
{
    unsigned int bind_idx = CastToUint(m_resource_descriptors.size());
    if (bind_idx >= m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    if (GetHeapType() != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        throw std::exception();

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);

    texture->CreateShaderResourceView(handle);

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptors.push_back(dynamic_cast<GpuResource*>(texture));
    return bind_idx;
}

unsigned int DescriptorHeap::Bind(RenderTargetTexture* texture)
{
    unsigned int bind_idx = CastToUint(m_resource_descriptors.size());
    if (bind_idx >= m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);

    switch (GetHeapType()) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        texture->CreateShaderResourceView(handle);
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        texture->CreateRenderTargetView(handle);
        break;
    default:
        throw std::exception("weird heap type for binding...");
    }

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptors.push_back(dynamic_cast<GpuResource*>(texture));
    return bind_idx;
}

unsigned int DescriptorHeap::Bind(DepthMapTexture* texture)
{
    unsigned int bind_idx = CastToUint(m_resource_descriptors.size());
    if (bind_idx >= m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);

    switch (GetHeapType()) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        texture->CreateShaderResourceView(handle);
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
        texture->CreateDepthStencilView(handle);
        break;
    default:
        throw std::exception("weird heap type for binding...");
    }

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptors.push_back(dynamic_cast<GpuResource*>(texture));
    return bind_idx;
}


unsigned int DescriptorHeap::Bind(RenderBuffer* render_buffer) 
{
    unsigned int bind_idx = CastToUint(m_resource_descriptors.size());

    if (bind_idx >= m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");
    
    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);

    render_buffer->CreateRenderTargetView(handle);

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptors.push_back(dynamic_cast<GpuResource*>(render_buffer));

    return bind_idx;
}

unsigned int DescriptorHeap::Bind(DepthBuffer* depth_buffer) 
{
    unsigned int bind_idx = CastToUint(m_resource_descriptors.size());

    if (bind_idx >= m_num_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCpuHandle(bind_idx);
    depth_buffer->CreateDepthStencilView(handle);

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptors.push_back(dynamic_cast<GpuResource*>(depth_buffer));

    return bind_idx;
}



/// FrameDescriptorHeap

FrameDescriptorHeap::FrameDescriptorHeap(unsigned int num_frames, unsigned int num_gui_descriptors) :
    m_num_frames(num_frames), m_num_frame_descriptors(0), m_resource_descriptors(num_frames), m_num_gui_descriptors(num_gui_descriptors), IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true)
{
}

FrameDescriptorHeap::FrameDescriptorHeap(unsigned int num_frames, unsigned int num_gui_descriptors, unsigned int num_descriptors) :
    m_num_frames(num_frames), m_num_frame_descriptors(num_descriptors), m_resource_descriptors(num_frames), m_num_gui_descriptors(num_gui_descriptors), IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true)
{
    Allocate(num_descriptors);
}

void FrameDescriptorHeap::Allocate(unsigned int num_descriptors) {
    m_num_frame_descriptors = num_descriptors;
    IDescriptorHeap::Allocate(num_descriptors * m_num_frames + m_num_gui_descriptors);
}


D3D12_GPU_DESCRIPTOR_HANDLE FrameDescriptorHeap::FindResourceHandle(IResourceType* resource, unsigned int frame_idx)
{
    const std::vector<IResourceType*>& descriptors = m_resource_descriptors[frame_idx];
    auto result = std::find(descriptors.begin(), descriptors.end(), resource);
    if (result == descriptors.end())
        throw std::exception("Resource not contained in descriptor heap");

    unsigned int bind_idx = result - descriptors.begin();
    return GetGpuHandle(frame_idx, bind_idx);

}

unsigned int FrameDescriptorHeap::Bind(IShaderResource* texture, unsigned int frame_idx) {
    unsigned int frame_bind_idx = CastToUint(m_resource_descriptors[frame_idx].size());

    if (frame_bind_idx >= m_num_frame_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = GetCpuHandle(frame_idx, frame_bind_idx);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = GetGpuHandle(frame_idx, frame_bind_idx);
    texture->BindShaderResourceView(frame_idx, cpu_handle, gpu_handle);

    // Cache in map that texture is bound at bind_idx
    m_resource_descriptors[frame_idx].push_back(dynamic_cast<IResourceType*>(texture));

    return frame_bind_idx;
}

unsigned int FrameDescriptorHeap::Bind(UploadBuffer* constant_buffer, unsigned int frame_idx)
{
    unsigned int frame_bind_idx = CastToUint(m_resource_descriptors[frame_idx].size());

    if (frame_bind_idx >= m_num_frame_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = GetCpuHandle(frame_idx, frame_bind_idx);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = GetGpuHandle(frame_idx, frame_bind_idx);
    constant_buffer->CreateConstantBufferView(cpu_handle, gpu_handle);

    // Cache in map that constant_buffer is bound at bind_idx
    m_resource_descriptors[frame_idx].push_back(dynamic_cast<IResourceType*>(constant_buffer));

    return frame_bind_idx;
}

unsigned int FrameDescriptorHeap::Bind(ImguiResource* resource)
{
    unsigned int gui_bind_idx = CastToUint(m_gui_descriptors.size());

    if (gui_bind_idx >= m_num_gui_descriptors)
        throw std::exception("Already reached max amount of descriptors in heap");

    unsigned int bind_idx = gui_bind_idx + m_num_frames * m_num_frame_descriptors;

    // Cache in map that resource is bound at bind_idx
    m_gui_descriptors.push_back(dynamic_cast<GpuResource*>(resource));

    return bind_idx;
}
