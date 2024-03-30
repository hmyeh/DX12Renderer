#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>

#include <vector>


// Forward Declarations
class CommandList;

// Resources which need to be uploaded to GPU and are not changed afterwards via CPU side
class GpuResource {
protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    // Naive way of tracking resource state
    //D3D12_RESOURCE_STATES m_resource_state = D3D12_RESOURCE_STATE_COMMON;

public:
    GpuResource() {}

    virtual ~GpuResource() { Destroy(); }


    void Destroy() { m_resource = nullptr; }

    ID3D12Resource* GetResource() { return m_resource.Get(); }
};


// Used to upload texture and mesh data to the GPU via Commited resources
class UploadBuffer : public GpuResource {
public:
    UploadBuffer() {

    }

    void Create(size_t buffer_size);
};


// Define vertex
struct ScreenVertex
{
    DirectX::XMFLOAT2 position;
    DirectX::XMFLOAT2 texture_coord;
};


struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texture_coord;
    DirectX::XMFLOAT3 normal;
};

// Concept to ensure vertex type struct is used for Meshes
template<class T>
concept IsVertex = std::is_class_v<Vertex> || std::is_class_v<ScreenVertex>;

template<class T>
concept IsIndex = std::is_unsigned_v<T>;

template <class T>
class GpuBuffer : public GpuResource {
    size_t m_buffer_size;

public:
    GpuBuffer() : m_buffer_size(0) {}

    void Create(size_t num_elements);
    void Upload(CommandList& command_list, const std::vector<T>& buffer_data);


    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const requires IsVertex<T>;
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const requires IsIndex<T>;

};
