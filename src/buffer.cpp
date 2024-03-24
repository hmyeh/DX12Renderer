#include "buffer.h"

#include "utility.h"
#include "renderer.h"
#include "commandlist.h"

/// Upload Buffer

void UploadBuffer::Create(size_t buffer_size) 
{
    // Destroy any previous resource
    Destroy();

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
}


// Gpu Buffer

// Declare the GpuBuffer types to be used to avoid Linker errors https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class GpuBuffer<Vertex>;
template class GpuBuffer<uint32_t>;

template <class T>
void GpuBuffer<T>::Create(size_t resource_size) 
{
    // Destroy any previous resource
    Destroy();

    m_buffer_size = resource_size;

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

template <class T>
D3D12_VERTEX_BUFFER_VIEW GpuBuffer<T>::GetVertexBufferView() const 
{
    // Create the vertex buffer view.
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
    vertex_buffer_view.BufferLocation = m_resource->GetGPUVirtualAddress();
    vertex_buffer_view.SizeInBytes = m_buffer_size;
    vertex_buffer_view.StrideInBytes = sizeof(Vertex);
    return vertex_buffer_view;
}

template <class T>
D3D12_INDEX_BUFFER_VIEW GpuBuffer<T>::GetIndexBufferView() const 
{
    D3D12_INDEX_BUFFER_VIEW index_buffer_view;
    index_buffer_view.BufferLocation = m_resource->GetGPUVirtualAddress();
    index_buffer_view.Format = DXGI_FORMAT_R32_UINT; // should check template class T type
    index_buffer_view.SizeInBytes = m_buffer_size;
    return index_buffer_view;
}

