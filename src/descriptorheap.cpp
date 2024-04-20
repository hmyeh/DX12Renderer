#include "descriptorheap.h"

#include "renderer.h"
#include "utility.h"

DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, unsigned int num_descriptors, bool shader_visible) :
    m_heap_type(heap_type), m_num_descriptors(num_descriptors), m_shader_visible(shader_visible) 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = m_num_descriptors;//m_num_frame_descriptors * Renderer::s_num_frames; // 1 srv diffuse texture (will be changed every model/mesh) + 1 cbv * framecount
    heap_desc.Flags = m_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.Type = m_heap_type;
    ThrowIfFailed(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_descriptor_heap)));

    m_descriptor_size = device->GetDescriptorHandleIncrementSize(m_heap_type);
}
