#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

#include <vector>

#include "commandqueue.h"


//// Vertex data for a colored cube.
struct VertexPosColor
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Color;
};

//struct Vertex
//{
//    DirectX::XMFLOAT3 position;
//    DirectX::XMFLOAT2 texture_coord;
//    // Normal
//};


// Resources which need to be uploaded to GPU and are not changed afterwards via CPU side
class GpuResource {
protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    // Naive way of tracking resource state
    //D3D12_RESOURCE_STATES m_resource_state = D3D12_RESOURCE_STATE_COMMON;

public:
    GpuResource() {

    }

    ~GpuResource() {
        Destroy();
    }

    void Destroy() {
        m_resource = nullptr;
    }

    ID3D12Resource* GetResource() { return m_resource.Get(); }
};


class UploadBuffer : public GpuResource {
public:
    UploadBuffer() {

    }

    void Create(size_t buffer_size);

    

};

// template class
template <class T>
class GpuBuffer : public GpuResource {
    uint32_t m_buffer_size;
    
public:
    GpuBuffer() : m_buffer_size(0) {

    }


    void Create(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& command_list, UploadBuffer& upload_buffer, const std::vector<T>& buffer_data);


    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const {
        // Create the vertex buffer view.
        D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
        vertex_buffer_view.BufferLocation = m_resource->GetGPUVirtualAddress();
        vertex_buffer_view.SizeInBytes = m_buffer_size;
        vertex_buffer_view.StrideInBytes = sizeof(VertexPosColor);
        return vertex_buffer_view;
    }

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const {
        D3D12_INDEX_BUFFER_VIEW index_buffer_view;
        index_buffer_view.BufferLocation = m_resource->GetGPUVirtualAddress();
        index_buffer_view.Format = DXGI_FORMAT_R32_UINT; // should check template class T type
        index_buffer_view.SizeInBytes = m_buffer_size;
        return index_buffer_view;
    }
    
};


class Mesh {
private:
    GpuBuffer<VertexPosColor> m_vertex_buffer;
    GpuBuffer<uint32_t> m_index_buffer;

    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
    D3D12_INDEX_BUFFER_VIEW m_index_buffer_view;

    std::vector<VertexPosColor> m_verts;
    std::vector<uint32_t> m_inds;


public:
    Mesh(const std::vector<VertexPosColor>& verts, const std::vector<uint32_t>& inds) : 
        m_verts(verts), m_inds(inds), m_vertex_buffer_view{}, m_index_buffer_view{}
    {

    }

    DirectX::XMMATRIX GetModelMatrix() const { return DirectX::XMMatrixIdentity(); }

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertex_buffer_view; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_index_buffer_view; }

    size_t GetNumIndices() const { return m_inds.size(); }

    // Load data from CPU -> GPU
    void Load(CommandQueue* command_queue);

    // Hardcoded for now
    static Mesh Read();
};

