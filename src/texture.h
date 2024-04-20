#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <DirectXTex.h>

#include <vector>
#include <map>
#include <string>

#include "commandqueue.h"
#include "buffer.h"
#include "descriptorheap.h"

class Texture : public GpuResource {
private:
    DirectX::ScratchImage m_image;
    DirectX::TexMetadata m_metadata;

    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsv_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_srv_handle;

public:
    Texture() : m_metadata{}, m_rtv_handle{}, m_dsv_handle{}, m_srv_handle{} {}

    void SetRtvHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& rtv_handle) { m_rtv_handle = rtv_handle; }
    void SetDsvHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& dsv_handle) { m_dsv_handle = dsv_handle; }
    void SetSrvHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle) { m_srv_handle = srv_handle; }

    // create the resource
    // depth = arraysize
    void Create(DirectX::TEX_DIMENSION dimensions, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void CreateViews();

    void Upload(CommandList& command_list);

    void Bind(const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_desc_handle);
    void Read(const std::wstring& file_name);
};


class TextureLibrary {
public:
    // for allocating the texture descriptors of different types (non-shader visible)
    std::unique_ptr<DescriptorHeap> m_srv_heap;
    std::unique_ptr<DescriptorHeap> m_rtv_heap;
    std::unique_ptr<DescriptorHeap> m_dsv_heap;

    // for now static since using the same samplers
    static std::unique_ptr<DescriptorHeap> s_sampler_heap;

    std::map<std::wstring, std::unique_ptr<Texture>> m_srv_texture_map;
    std::vector<std::unique_ptr<Texture>> m_rtv_textures; // might be changed later to have name/id associated
    std::vector<std::unique_ptr<Texture>> m_dsv_textures;

    // Commandqueue for copying
    CommandQueue m_command_queue;

    TextureLibrary();

    ~TextureLibrary() {
        m_command_queue.Flush();
    }

    // Lazily allocate the descriptors
    void AllocateDescriptors();

    Texture* CreateTexture(std::wstring file_name);
    Texture* CreateRenderTargetTexture(DXGI_FORMAT format, uint32_t width, uint32_t height);
    Texture* CreateDepthTexture(DXGI_FORMAT format, uint32_t width, uint32_t height);

    // Loading all textures to GPU at once
    void Load();

    size_t GetNumTextures() const { return m_srv_texture_map.size(); }

    // Below functions could maybe be static, since they are quite general?
    static DescriptorHeap* GetSamplerHeap() { return s_sampler_heap.get(); }
    static D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHeapGpuHandle() { return s_sampler_heap->GetGpuHandle(); }

private:
    // Create static texture samplers
    static std::unique_ptr<DescriptorHeap> CreateSamplers();
};

