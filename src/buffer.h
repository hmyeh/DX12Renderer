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

    // Descriptor handles
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsv_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cbv_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_srv_handle;

public:
    GpuResource() : m_resource_state(D3D12_RESOURCE_STATE_COMMON), m_rtv_handle{}, m_dsv_handle{}, m_cbv_handle{}, m_srv_handle{} {}

    virtual ~GpuResource() { Destroy(); }


    void Destroy() { m_resource.Reset(); }//m_resource = nullptr; }

    ID3D12Resource* GetResource() { return m_resource.Get(); }

    // TODO: look at how to make this work for multithreaded situation
    void TransitionResourceState(CommandList& command_list, D3D12_RESOURCE_STATES updated_state);
    D3D12_RESOURCE_STATES GetResourceState() const { return m_resource_state; }

    // Descriptor functions
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return m_rtv_handle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return m_dsv_handle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCBVHandle() const { return m_cbv_handle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return m_srv_handle; }

    void BindRenderTargetView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr);
    void BindDepthStencilView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr);
    void BindConstantBufferView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle);
    void BindShaderResourceView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);
};


// Used to upload texture and mesh data to the GPU via Commited resources
class UploadBuffer : public GpuResource {
public:
    UploadBuffer() {

    }

    void Create(size_t buffer_size);
    void Map(unsigned int subresource, const D3D12_RANGE* read_range, void** buffer_WO);
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

// Interface for rendertargets
class IRenderTarget {
public:
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const = 0;// GetRTVHandle / dsv handle all cpu handles
    virtual void Clear(CommandList& command_list) = 0;
    // also need a function to transition resource to SRV state if used as shader variable
};

class RenderBuffer : public GpuResource, public IRenderTarget {
private:
    uint64_t m_fence_value; 

public:
    RenderBuffer() : m_fence_value(0) {}

    void Create(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swap_chain, unsigned int frame_idx);

    uint64_t GetFenceValue() const { return m_fence_value; }
    void SetFenceValue(uint64_t updated_fence_value) {
        if (updated_fence_value < m_fence_value)
            throw std::exception("this should not be happening");
        m_fence_value = updated_fence_value;
    }

    virtual void Clear(CommandList& command_list) override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const override { return GetRTVHandle(); }

    void Present(CommandList& command_list) { TransitionResourceState(command_list, D3D12_RESOURCE_STATE_PRESENT); }
};

class DepthBuffer : public GpuResource, public IRenderTarget {
public:
    DepthBuffer() : GpuResource() {}

    void Create(uint32_t width, uint32_t height);

    virtual void Clear(CommandList& command_list) override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const override { return GetDSVHandle(); }

    void Resize(uint32_t width, uint32_t height) {
        Destroy();
        Create(width, height);
        //Bind(DescriptorType::DepthStencilView, m_dsv_handle);// not sure if needed
    }
};
