#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <DirectXTex.h>

#include <vector>
#include <map>
#include <string>

#include "buffer.h"
#include "commandqueue.h"
#include "descriptorheap.h"


class Texture : public GpuResource, public IRenderTarget {
private:
    DirectX::ScratchImage m_image;
    DirectX::TexMetadata m_metadata;
    D3D12_RESOURCE_FLAGS m_flags;

public:
    Texture() : m_metadata{}, m_flags(D3D12_RESOURCE_FLAG_NONE) {}

    // create the resource
    // depth = arraysize
    void Create(DirectX::TEX_DIMENSION dimensions, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void Upload(CommandList& command_list);

    void Read(const std::wstring& file_name);

    // IRenderTarget implementations
    virtual void Clear(CommandList& command_list) override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const override;

    // Needs method to use as SRV after being used as rendertarget
    void UseShaderResource(CommandList& command_list) { TransitionResourceState(command_list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); }// TODO: figure out D3D12_RESOURCE_STATE_DEPTH_READ is needed for depth stencil view reading in shader
};


class TextureLibrary {
public:
    // for allocating the texture descriptors of different types (non-shader visible)
    DescriptorHeap m_srv_heap;
    DescriptorHeap m_rtv_heap;
    DescriptorHeap m_dsv_heap;

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

    void Bind(FrameDescriptorHeap* descriptor_heap);
    void Bind(FrameDescriptorHeap* descriptor_heap, unsigned int frame_idx);

    Texture* CreateTexture(std::wstring file_name);
    Texture* CreateRenderTargetTexture(DXGI_FORMAT format, uint32_t width, uint32_t height);
    Texture* CreateDepthTexture(DXGI_FORMAT format, uint32_t width, uint32_t height);

    // Loading all textures to GPU at once
    void Load();

    size_t GetNumTextures() const { return m_srv_texture_map.size() + m_rtv_textures.size() + m_dsv_textures.size(); }

    // Below functions could maybe be static, since they are quite general?
    static DescriptorHeap* GetSamplerHeap() { return s_sampler_heap.get(); }
    static D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHeapGpuHandle() { return s_sampler_heap->GetGpuHandle(); }

private:
    // Create static texture samplers
    static std::unique_ptr<DescriptorHeap> CreateSamplers();
};

