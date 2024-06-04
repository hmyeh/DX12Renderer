#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

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

    virtual void Destroy() { m_resource.Reset(); }

    ID3D12Resource* GetResource() { return m_resource.Get(); }

    // TODO: look at how to make this work for multithreaded situation
    void TransitionResourceState(CommandList& command_list, D3D12_RESOURCE_STATES updated_state);
    D3D12_RESOURCE_STATES GetResourceState() const { return m_resource_state; }
};

class IResourceType {
    // dummy class
};

class IShaderResource : public IResourceType {
private:
    // cpu only handle
    D3D12_CPU_DESCRIPTOR_HANDLE m_srv_handle; // local non-shadervisible heap handle
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_shader_visible_cpu_handles;
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_shader_visible_gpu_handles;

protected:
    virtual void CreateShaderResourceView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);

public:
    IShaderResource();
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderCPUHandle() const { return m_srv_handle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetShaderGPUHandle(unsigned int frame_idx) const { return m_shader_visible_gpu_handles[frame_idx]; }
    void BindShaderResourceView(unsigned int frame_idx, const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle);
    // in case resource has been reset and recreated for resize
    void ResourceChanged(ID3D12Resource* resource);
};


//
class IConstantBufferResource : public IResourceType {
private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_cbv_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_cbv_gpu_handle;

protected:
    void CreateConstantBufferView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle);

public:
    D3D12_CPU_DESCRIPTOR_HANDLE GetBufferCPUHandle() const { return m_cbv_cpu_handle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetBufferGPUHandle() const { return m_cbv_gpu_handle; }
};

// Used to upload texture and mesh data to the GPU via Commited resources
class UploadBuffer : public GpuResource, public IConstantBufferResource {
public:
    UploadBuffer() : GpuResource(), IConstantBufferResource() {

    }

    void Create(size_t buffer_size);
    void Map(unsigned int subresource, const D3D12_RANGE* read_range, void** buffer_WO);

    void CreateConstantBufferView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle) { IConstantBufferResource::CreateConstantBufferView(m_resource.Get(), handle, gpu_handle); }
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
