#pragma once

#include <d3d12.h>
#include <d3dx12.h>

#include <map>

// Due to Windows.h import
#define NOMINMAX

// Forward declarations
class Texture;
class RenderBuffer;
class DepthBuffer;
class UploadBuffer;
class GpuResource;


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

    //unsigned int m_num_bound_descriptors;
public:

    virtual void Allocate(unsigned int num_descriptors);

    // TODO: Add checks if not yet allocated
    // get descriptor heap 
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
    // map to bookkeep what pointer texture/renderbuffer/depthbuffer is bound at address unsigned int in the descriptorheap
    std::map<GpuResource*, unsigned int> m_resource_descriptor_map;

public:
    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, bool shader_visible = false) : IDescriptorHeap(heap_type, shader_visible) {}

    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, unsigned int num_descriptors, bool shader_visible = false) : IDescriptorHeap(heap_type, num_descriptors, shader_visible) {}


    D3D12_GPU_DESCRIPTOR_HANDLE FindResourceHandle(GpuResource* resource);

    unsigned int Bind(Texture* texture);
    unsigned int Bind(RenderBuffer* render_buffer);
    unsigned int Bind(DepthBuffer* depth_buffer);
};

// Special descriptorheap for cbv srv uav for per frame stuff
class FrameDescriptorHeap : public IDescriptorHeap { 
private:
    unsigned int m_num_frames;
    unsigned int m_num_frame_descriptors;

    std::vector<std::map<GpuResource*, unsigned int> > m_resource_descriptor_maps;

public:
    FrameDescriptorHeap(unsigned int num_frames);
    FrameDescriptorHeap(unsigned int num_frames, unsigned int num_descriptors);

    virtual void Allocate(unsigned int num_descriptors);
    
    D3D12_GPU_DESCRIPTOR_HANDLE FindResourceHandle(GpuResource* resource, unsigned int frame_idx);
    
    unsigned int Bind(Texture* texture, unsigned int frame_idx);
    unsigned int Bind(UploadBuffer* constant_buffer, unsigned int frame_idx);// UploadBuffer used as ConstantBuffer

};
