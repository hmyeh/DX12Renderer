#include "buffer.h"

#include "utility.h"
#include "renderer.h"
#include "commandlist.h"

/// Gpu Resource

void GpuResource::TransitionResourceState(CommandList& command_list, D3D12_RESOURCE_STATES updated_state) {
    if (updated_state == m_resource_state)
        throw std::exception("Transition to same state..");
    command_list.ResourceBarrier(m_resource.Get(), m_resource_state, updated_state);
    m_resource_state = updated_state; // state should only actually change after execution of the command list....
}

// IShader Resource

IShaderResource::IShaderResource() : m_shader_visible_cpu_handles(Renderer::s_num_frames), m_shader_visible_gpu_handles(Renderer::s_num_frames), m_srv_handle{}  {}

void IShaderResource::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    device->CreateShaderResourceView(resource, desc, handle);
    m_srv_handle = handle;
}

void IShaderResource::BindShaderResourceView(unsigned int frame_idx, const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle)
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    if (!m_srv_handle.ptr)
        throw std::exception();

    device->CopyDescriptorsSimple(1, cpu_handle, m_srv_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Cache the descriptor handles
    m_shader_visible_cpu_handles[frame_idx] = cpu_handle;
    m_shader_visible_gpu_handles[frame_idx] = gpu_handle;
}

void IShaderResource::ResourceChanged(ID3D12Resource* resource)
{
    // recreate
    CreateShaderResourceView(resource, m_srv_handle);

    for (unsigned int frame_idx = 0; frame_idx < Renderer::s_num_frames; ++frame_idx) {
        if (m_shader_visible_cpu_handles[frame_idx].ptr) {
            BindShaderResourceView(frame_idx, m_shader_visible_cpu_handles[frame_idx], m_shader_visible_gpu_handles[frame_idx]);
        }
    }
}

// IConstant Buffer Resource

void IConstantBufferResource::CreateConstantBufferView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle)
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    if (m_cbv_cpu_handle.ptr) {
        device->CopyDescriptorsSimple(1, handle, m_cbv_cpu_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    else {
        D3D12_RESOURCE_DESC resource_desc = resource->GetDesc();
        unsigned int array_size = (resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? resource_desc.DepthOrArraySize : 1u;
        unsigned int num_subresources = resource_desc.MipLevels * array_size; // * PlaneCount (for constant buffer assume DXGI_FORMAT_UNKNOWN)

        uint64_t buffer_size;
        device->GetCopyableFootprints(&resource_desc, 0, num_subresources, 0, nullptr, nullptr, nullptr, &buffer_size);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
        cbv_desc.BufferLocation = resource->GetGPUVirtualAddress();
        cbv_desc.SizeInBytes = buffer_size;    // CB size is required to be 256-byte aligned.

        device->CreateConstantBufferView(&cbv_desc, handle);
        m_cbv_cpu_handle = handle;
        m_cbv_gpu_handle = gpu_handle;
    }
}



/// Upload Buffer

void UploadBuffer::Create(size_t buffer_size) 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto upload_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto upload_heap_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

    ThrowIfFailed(device->CreateCommittedResource(
        &upload_heap_props,
        D3D12_HEAP_FLAG_NONE,
        &upload_heap_resource_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)));

    // Set appropriate initial resource state
    m_resource_state = D3D12_RESOURCE_STATE_GENERIC_READ;
    m_resource->SetName(L"Upload Buffer");
}

void UploadBuffer::Map(unsigned int subresource, const D3D12_RANGE* read_range, void** buffer_WO)
{
    ThrowIfFailed(m_resource->Map(0, read_range, buffer_WO));
}


// Gpu Buffer

template <class T>
void GpuBuffer<T>::Create(size_t num_elements) 
{
    m_buffer_size = num_elements * sizeof(T);

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto heap_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(m_buffer_size);
    // Create a committed resource for the GPU resource in a default heap.
    ThrowIfFailed(device->CreateCommittedResource(
        &heap_props,
        D3D12_HEAP_FLAG_NONE,
        &heap_resource_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_resource)));

    // Set appropriate initial resource state
    m_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
    m_resource->SetName(L"Templated Gpu Buffer");
}

template <class T>
void GpuBuffer<T>::Upload(CommandList& command_list, const std::vector<T>& buffer_data) 
{
    if (m_buffer_size != buffer_data.size() * sizeof(T))
        throw std::exception();

    // Upload function commandlist, uploadbuffer
    D3D12_SUBRESOURCE_DATA subresource_data = {};
    subresource_data.pData = std::data(buffer_data);
    subresource_data.RowPitch = m_buffer_size;
    subresource_data.SlicePitch = subresource_data.RowPitch;

    command_list.UploadBufferData(m_buffer_size, this->GetResource(), 1, &subresource_data);
}

// c++20 requires clause
template <class T>
D3D12_VERTEX_BUFFER_VIEW GpuBuffer<T>::GetVertexBufferView() const requires IsVertex<T> 
{
    // Create the vertex buffer view.
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
    vertex_buffer_view.BufferLocation = m_resource->GetGPUVirtualAddress();
    vertex_buffer_view.SizeInBytes = CastToUint(m_buffer_size);
    vertex_buffer_view.StrideInBytes = sizeof(T);
    return vertex_buffer_view;
}

template <class T>
D3D12_INDEX_BUFFER_VIEW GpuBuffer<T>::GetIndexBufferView() const requires IsIndex<T>
{
    D3D12_INDEX_BUFFER_VIEW index_buffer_view{};
    index_buffer_view.BufferLocation = m_resource->GetGPUVirtualAddress();
    index_buffer_view.Format = DXGI_FORMAT_R32_UINT; // Should check Type num bits
    index_buffer_view.SizeInBytes = CastToUint(m_buffer_size);
    return index_buffer_view;
}

// Declare the GpuBuffer types to be used to avoid Linker errors https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class GpuBuffer<ScreenVertex>;
template class GpuBuffer<Vertex>;
template class GpuBuffer<uint32_t>;
