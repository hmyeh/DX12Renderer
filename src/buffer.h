#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>

#include <vector>


// Forward Declarations
class CommandList;

// Resources which need to be uploaded to GPU and are not changed afterwards via CPU side
class GpuResource {
protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    // Naive way of tracking resource state
    D3D12_RESOURCE_STATES m_resource_state;

public:
    GpuResource() : m_resource_state(D3D12_RESOURCE_STATE_COMMON) {}

    virtual ~GpuResource() { Destroy(); }


    void Destroy() { m_resource.Reset(); }//m_resource = nullptr; }

    ID3D12Resource* GetResource() { return m_resource.Get(); }

    // naive way to do this
    void TransitionResourceState(CommandList& command_list, D3D12_RESOURCE_STATES updated_state);
    D3D12_RESOURCE_STATES GetResourceState() const { return m_resource_state; }
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

class RenderBuffer : public GpuResource {
private:
    uint64_t m_fence_value; // should i keep the fence value in here or not?...

    D3D12_CPU_DESCRIPTOR_HANDLE m_desc_handle;

public:
    RenderBuffer() : m_fence_value(0) {}

    void Create(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swap_chain, unsigned int frame_idx, const D3D12_CPU_DESCRIPTOR_HANDLE& rtv_handle);

    uint64_t GetFenceValue() const { return m_fence_value; }
    void SetFenceValue(uint64_t updated_fence_value) {
        if (updated_fence_value < m_fence_value)
            throw std::exception("this should not be happening");
        m_fence_value = updated_fence_value;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescHandle() const { return m_desc_handle; }

    void Clear(CommandList& command_list);
};

class DepthBuffer : public GpuResource {
private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_desc_handle;

public:
    DepthBuffer() {}

    void Create(uint32_t width, uint32_t height, const D3D12_CPU_DESCRIPTOR_HANDLE& dsv_handle);

    void Clear(CommandList& command_list);

    void Resize(uint32_t width, uint32_t height) {
        Destroy();
        Create(width, height, m_desc_handle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescHandle() const { return m_desc_handle; }
};
