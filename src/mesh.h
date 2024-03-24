#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>
//#include <DirectXTex.h>


#include <vector>

#include "commandqueue.h"
#include "buffer.h"
#include "texture.h"



class Mesh {
private:
    GpuBuffer<Vertex> m_vertex_buffer;
    GpuBuffer<uint32_t> m_index_buffer;

    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
    D3D12_INDEX_BUFFER_VIEW m_index_buffer_view;

    std::vector<Vertex> m_verts;
    std::vector<uint32_t> m_inds;

    Texture* m_diffuse_tex;


public:
    Mesh(const std::vector<Vertex>& verts, const std::vector<uint32_t>& inds, Texture* diffuse_tex) :
        m_verts(verts), m_inds(inds), m_vertex_buffer_view{}, m_index_buffer_view{}, m_diffuse_tex(diffuse_tex)
    {

    }

    DirectX::XMMATRIX GetModelMatrix() const { return DirectX::XMMatrixIdentity(); }

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertex_buffer_view; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_index_buffer_view; }

    size_t GetNumIndices() const { return m_inds.size(); }

    // Load data from CPU -> GPU
    void Load(CommandQueue* command_queue);

    Texture* GetDiffuseTex() const { return m_diffuse_tex; }
    void Bind(const D3D12_CPU_DESCRIPTOR_HANDLE& bind_handle) const {
        // only diffuse tex for now
        m_diffuse_tex->Bind(bind_handle);
    }
    // Hardcoded for now
    static Mesh Read(Texture* dif_tex);
};

