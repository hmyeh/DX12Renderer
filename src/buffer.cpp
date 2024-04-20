#include "buffer.h"

#include "utility.h"
#include "renderer.h"
#include "commandlist.h"

/// Gpu Resource

// naive way to do this
void GpuResource::TransitionResourceState(CommandList& command_list, D3D12_RESOURCE_STATES updated_state) {
    command_list.ResourceBarrier(m_resource.Get(), m_resource_state, updated_state);
    m_resource_state = updated_state; // state should only actually change after execution of the command list....
}


/// Upload Buffer

void UploadBuffer::Create(size_t buffer_size) 
{
    // Destroy any previous resource
    //Destroy();

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

// RenderBuffer
void RenderBuffer::Create(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swap_chain, unsigned int frame_idx, const D3D12_CPU_DESCRIPTOR_HANDLE& rtv_handle) {
    if (frame_idx > Renderer::s_num_frames)
        throw std::exception("Illegal too high framecount");

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    ThrowIfFailed(swap_chain->GetBuffer(frame_idx, IID_PPV_ARGS(&m_resource)));
    // Set to default resource state
    m_resource_state = D3D12_RESOURCE_STATE_PRESENT;

    device->CreateRenderTargetView(m_resource.Get(), nullptr, rtv_handle);
    m_desc_handle = rtv_handle;
}

void RenderBuffer::Clear(CommandList& command_list) {
    // Clearing the buffer means set state to rendertarget
    TransitionResourceState(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);
    FLOAT clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };
    command_list.ClearRenderTargetView(m_desc_handle, clear_color);
}

// DepthBuffer
void DepthBuffer::Create(uint32_t width, uint32_t height, const D3D12_CPU_DESCRIPTOR_HANDLE& dsv_handle) {
    // Resize screen dependent resources.
// Create a depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    ThrowIfFailed(device->CreateCommittedResource(
        &heap_props,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_resource)
    ));

    // Update resource state
    m_resource_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(m_resource.Get(), &dsv, dsv_handle);
    m_desc_handle = dsv_handle;
}

void DepthBuffer::Clear(CommandList& command_list) {
    float depth = 1.0f;
    command_list.ClearDepthStencilView(m_desc_handle, depth);
}
