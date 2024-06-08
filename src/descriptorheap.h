#pragma once

#include <d3d12.h>
#include <d3dx12.h>

#include <map>

// Forward declarations
class IResourceType;
class IShaderResource;
class IConstantBufferResource;
class Texture;
class RenderTargetTexture;
class DepthMapTexture;
class RenderBuffer;
class DepthBuffer;
class UploadBuffer;
class GpuResource;
class ImguiResource;


class IDescriptorHeap {
protected:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;
    unsigned int m_descriptor_size;
    bool m_shader_visible;
    
protected:
    unsigned int m_num_descriptors;

protected:
    IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, bool shader_visible);
    IDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, unsigned int num_descriptors, bool shader_visible);

public:
    void Allocate(unsigned int num_descriptors);

    // TODO: Add checks if not yet allocated
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(unsigned int offset = 0) {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), offset * m_descriptor_size);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(unsigned int offset = 0) {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptor_heap->GetGPUDescriptorHandleForHeapStart(), offset * m_descriptor_size);
    }

    ID3D12DescriptorHeap* GetDescriptorHeap() { return m_descriptor_heap.Get(); }

    bool IsShaderVisible() const { return m_shader_visible; }
    D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_heap_type; }
};

class DescriptorHeap : public IDescriptorHeap {
private:
    // Keep track of the bound resource descriptors for debugging
    std::vector<GpuResource*> m_resource_descriptors;

public:
    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, bool shader_visible = false) : IDescriptorHeap(heap_type, shader_visible) {}
    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, unsigned int num_descriptors, bool shader_visible = false) : IDescriptorHeap(heap_type, num_descriptors, shader_visible) {}

    D3D12_GPU_DESCRIPTOR_HANDLE FindResourceHandle(GpuResource* resource);

    // Bind each resource type to the descriptor heap
    unsigned int Bind(Texture* texture);
    unsigned int Bind(RenderTargetTexture* texture);
    unsigned int Bind(DepthMapTexture* texture);
    unsigned int Bind(RenderBuffer* render_buffer);
    unsigned int Bind(DepthBuffer* depth_buffer);

    void Reset()  { m_resource_descriptors.clear(); }
};

// Special per frame descriptorheap for Constant buffers, Shader Resources and Unordered Access views
class FrameDescriptorHeap : public IDescriptorHeap { 
private:
    unsigned int m_num_frames;
    unsigned int m_num_frame_descriptors;

    // Store imgui descriptors separately
    unsigned int m_num_gui_descriptors;
    std::vector<GpuResource*> m_gui_descriptors;

    std::vector<std::vector<IResourceType*> > m_resource_descriptors;

    using IDescriptorHeap::Allocate;
    using IDescriptorHeap::GetCpuHandle;
    using IDescriptorHeap::GetGpuHandle;


public:
    FrameDescriptorHeap(unsigned int num_frames, unsigned int num_gui_descriptors);
    FrameDescriptorHeap(unsigned int num_frames, unsigned int num_gui_descriptors, unsigned int num_descriptors);

    void Allocate(unsigned int num_descriptors);
    
    D3D12_GPU_DESCRIPTOR_HANDLE FindResourceHandle(IResourceType* resource, unsigned int frame_idx);
    
    // IMGUI slots reserved at start, then normal srv slots
    unsigned int Bind(IShaderResource* shader_resource, unsigned int frame_idx);
    unsigned int Bind(UploadBuffer* constant_buffer, unsigned int frame_idx);// UploadBuffer used as ConstantBuffer

    // Imgui specific methods
    unsigned int Bind(ImguiResource* resource);
    D3D12_CPU_DESCRIPTOR_HANDLE GetImguiCpuHandle(unsigned int offset = 0) {
        return IDescriptorHeap::GetCpuHandle(offset);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetImguiGpuHandle(unsigned int offset = 0) {
        return IDescriptorHeap::GetGpuHandle(offset);
    }


    void Reset() {
        for (auto& descs : m_resource_descriptors)
            descs.clear();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(unsigned int frame_idx = 0, unsigned int offset = 0) {
        return IDescriptorHeap::GetCpuHandle(offset + frame_idx * m_num_frame_descriptors + m_num_gui_descriptors);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(unsigned int frame_idx = 0, unsigned int offset = 0) {
        return IDescriptorHeap::GetGpuHandle(offset + frame_idx * m_num_frame_descriptors + m_num_gui_descriptors);
    }

};
