#include "mesh.h"

#include <d3dx12.h>

#include "utility.h"
#include "renderer.h"


/// Upload Buffer

void UploadBuffer::Create(size_t buffer_size) {
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

template <class T>
void GpuBuffer<T>::Create(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& command_list, UploadBuffer& upload_buffer, const std::vector<T>& buffer_data) {
    // Destroy any previous resource
    Destroy();

    m_buffer_size = buffer_data.size() * sizeof(T);

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

    D3D12_SUBRESOURCE_DATA sub_resource_data = {};
    sub_resource_data.pData = std::data(buffer_data);
    sub_resource_data.RowPitch = m_buffer_size;
    sub_resource_data.SlicePitch = sub_resource_data.RowPitch;

    UpdateSubresources(command_list.Get(), this->GetResource(), upload_buffer.GetResource(), 0, 0, 1, &sub_resource_data);
}




// Mesh

void Mesh::Load(CommandQueue* command_queue) {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto command_list = command_queue->getCommandList();

    UploadBuffer upload_vb;
    upload_vb.Create(m_verts.size() * sizeof(VertexPosColor));
    m_vertex_buffer.Create(command_list, upload_vb, m_verts);
    m_vertex_buffer_view = m_vertex_buffer.GetVertexBufferView();

    UploadBuffer upload_ib;
    upload_ib.Create(m_inds.size() * sizeof(uint32_t));
    m_index_buffer.Create(command_list, upload_ib, m_inds);
    m_index_buffer_view = m_index_buffer.GetIndexBufferView();

    //m_vb.Load(command_list);
    //m_ib.Load(command_list);

    auto fence_value = command_queue->executeCommandList(command_list);
    command_queue->waitForFenceValue(fence_value);
}


Mesh Mesh::Read() {
    std::vector<VertexPosColor> vertices{
        { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
        { DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
        { DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
        { DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
        { DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
        { DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
        { DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
        { DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
    };

    std::vector<uint32_t> indices{
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    return Mesh(vertices, indices);
}
