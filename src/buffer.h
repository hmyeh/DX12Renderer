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


class IBuffer {
public:
    virtual void Create(size_t buffer_size) = 0;
};

// Used to upload texture and mesh data to the GPU via Commited resources
class UploadBuffer : public GpuResource, public IBuffer {
public:
    UploadBuffer() {

    }

    virtual void Create(size_t buffer_size) override;
};


// Define vertex
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texture_coord;
    // Normal
};


template <class T>
class GpuBuffer : public GpuResource, public IBuffer {
    uint32_t m_buffer_size;

public:
    GpuBuffer() : m_buffer_size(0) {}

    virtual void Create(size_t resource_size) override;
    void Upload(CommandList& command_list, const std::vector<T>& buffer_data);


    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

};
