#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <DirectXTex.h>

#include <map>
#include <string>

#include "commandqueue.h"
#include "buffer.h"

class Texture : public GpuResource {
private:
    DirectX::ScratchImage m_image;
    DirectX::TexMetadata m_metadata;

    //D3D12_CPU_DESCRIPTOR_HANDLE m_rtv_handle;
    //D3D12_CPU_DESCRIPTOR_HANDLE m_dsv_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_srv_handle;

public:
    Texture(D3D12_CPU_DESCRIPTOR_HANDLE srv_handle) : m_srv_handle(srv_handle) {

    }

    // create the resource
    // depth = arraysize
    void Create(DirectX::TEX_DIMENSION dimensions, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void CreateViews();

    void Upload(CommandList& command_list);

    void Bind(const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_desc_handle);
    void Read(const std::wstring& file_name);
};


class TextureManager {
public:
    // for allocating the texture descriptors
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_desc_heap;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_sampler_heap;

    std::map<std::wstring, std::unique_ptr<Texture>> m_texture_map;

    unsigned int m_num_descriptors;
    unsigned int m_srv_descriptor_size;
    unsigned int m_num_free_descriptors;

    // Commandqueue for copying
    CommandQueue m_command_queue;

    TextureManager() : TextureManager(0) {} // should be a more sensible solution for this...
    TextureManager(unsigned int num_descriptors);

    ~TextureManager() {
        m_command_queue.Flush();
    }

    Texture* Read(std::wstring file_name);

    // Loading all textures to GPU at once
    void Load();

    unsigned int GetNumTextures() { return m_texture_map.size(); }

    ID3D12DescriptorHeap* GetSamplerHeap() { return m_sampler_heap.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHeapGpuHandle() const { return m_sampler_heap->GetGPUDescriptorHandleForHeapStart(); }

private:
    // Create static texture samplers
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateSamplers();
};

