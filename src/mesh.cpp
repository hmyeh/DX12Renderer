#include "mesh.h"

#include <d3dx12.h>

#include "utility.h"
#include "renderer.h"


// Mesh

void Mesh::Load(CommandQueue* command_queue) {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto command_list = command_queue->GetCommandList();

    m_vertex_buffer.Create(m_verts.size() * sizeof(Vertex));
    m_vertex_buffer.Upload(command_list, m_verts);
    m_vertex_buffer_view = m_vertex_buffer.GetVertexBufferView();

    m_index_buffer.Create(m_inds.size() * sizeof(uint32_t));
    m_index_buffer.Upload(command_list, m_inds);
    m_index_buffer_view = m_index_buffer.GetIndexBufferView();

    auto fence_value = command_queue->ExecuteCommandList(command_list);
    command_queue->WaitForFenceValue(fence_value);
}


Mesh Mesh::Read(Texture* dif_tex) {
    std::vector<Vertex> vertices{
        { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) }, // 0
        { DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) }, // 1
        { DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }, // 2
        { DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) }, // 3
        { DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) }, // 4
        { DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) }, // 5
        { DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }, // 6
        { DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) }  // 7
    };

    std::vector<uint32_t> indices{
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    return Mesh(vertices, indices, dif_tex);
}
