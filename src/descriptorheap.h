#pragma once

#include <d3d12.h>
#include <d3dx12.h>

class DescriptorHeap {
private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;
    unsigned int m_descriptor_size;
    unsigned int m_num_descriptors;
    bool m_shader_visible;

public:
    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, unsigned int num_descriptors, bool shader_visible = false);


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
