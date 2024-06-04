#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <DirectXTex.h>

#include <vector>
#include <map>
#include <string>

#include "buffer.h"
#include "rendertarget.h"
#include "commandqueue.h"
#include "descriptorheap.h"

// Base class ITexture 
class ITexture : public GpuResource, public IShaderResource {
protected:
    D3D12_RESOURCE_DESC m_resource_desc;
    D3D12_RESOURCE_FLAGS m_flags;
    D3D12_CLEAR_VALUE m_clear_value;
    bool m_use_clear_value;

public:
    ITexture() : m_flags(D3D12_RESOURCE_FLAG_NONE), m_resource_desc{}, m_use_clear_value(false), m_clear_value{}, IShaderResource() {}

    // create the resource
    // depth = arraysize
    void Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, const D3D12_CLEAR_VALUE& clear_value, bool use_clear_value = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    virtual void CreateShaderResourceView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr) { IShaderResource::CreateShaderResourceView(m_resource.Get(), handle, desc); }
    
    virtual void Resize(unsigned int width, unsigned int height) {
        Destroy();
        Create(m_resource_desc.Dimension, m_resource_desc.Format, width, height, m_resource_desc.DepthOrArraySize, m_resource_desc.MipLevels, m_clear_value, m_use_clear_value, m_flags);
        IShaderResource::ResourceChanged(m_resource.Get());
    }

    // Needs method to use as SRV after being used as rendertarget
    void UseShaderResource(CommandList& command_list) {  TransitionResourceState(command_list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); }
};


// Normal Texture class which reads from files
class Texture : public ITexture {
private:
    DirectX::ScratchImage m_image;
    DirectX::TexMetadata m_metadata;

private:
    ITexture::Create;

public:
    void Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void Upload(CommandList& command_list);

    void Read(const std::wstring& file_name);
};

// Render target texture
class RenderTargetTexture : public ITexture, public IRenderTarget {
private:
    ITexture::Create;

public:
    void Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    virtual void Resize(unsigned int width, unsigned int height) override { ITexture::Resize(width, height); IRenderTarget::ResourceChanged(m_resource.Get()); }

    virtual void ClearRenderTarget(CommandList& command_list) override;
    void CreateRenderTargetView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr) { IRenderTarget::CreateRenderTargetView(m_resource.Get(), handle, desc); }
    
};

// Depth map texture
class DepthMapTexture : public ITexture, public IDepthStencilTarget {
private:
    ITexture::Create;

public:
    void Create(D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    virtual void Resize(unsigned int width, unsigned int height) override { ITexture::Resize(width, height); IDepthStencilTarget::ResourceChanged(m_resource.Get()); }

public:
    virtual void ClearDepthStencil(CommandList& command_list) override;

    virtual void CreateShaderResourceView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr) override {
        DXGI_FORMAT shader_format = IDepthStencilTarget::TypelessToShaderFormat(m_resource_desc.Format);
        // Create the depth-stencil view.
        if (desc) {
            desc->Format = shader_format;
            ITexture::CreateShaderResourceView(handle, desc);
        }
        else {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
            srv.Format = shader_format;
            //srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv.Texture2D.MipLevels = m_resource_desc.MipLevels;
            
            // Map correct dimension
            switch (m_resource_desc.Dimension) {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                break;
            default:
                throw std::exception();
            }

            ITexture::CreateShaderResourceView(handle, &srv);
        }
    }

    void CreateDepthStencilView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr) {
        DXGI_FORMAT dsv_format = IDepthStencilTarget::TypelessToDepthStencilFormat(m_resource_desc.Format);

        // Create the depth-stencil view.
        if (desc) {
            desc->Format = dsv_format;
            IDepthStencilTarget::CreateDepthStencilView(m_resource.Get(), handle, desc);
        }
        else {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
            dsv.Format = dsv_format;
            dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv.Texture2D.MipSlice = 0;
            dsv.Flags = D3D12_DSV_FLAG_NONE;

            // Map correct dimension
            switch (m_resource_desc.Dimension) {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                break;
            default:
                throw std::exception();
            }

            IDepthStencilTarget::CreateDepthStencilView(m_resource.Get(), handle, &dsv);
        }
    }
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
    std::vector<std::unique_ptr<RenderTargetTexture>> m_rtv_textures; // might be changed later to have name/id associated
    std::vector<std::unique_ptr<DepthMapTexture>> m_dsv_textures;

    // Commandqueue for copying
    CommandQueue m_command_queue;

    // Cache Framedescriptorheap bound
    FrameDescriptorHeap* m_frame_descriptor_heap;

    TextureLibrary();

    ~TextureLibrary() {
        m_command_queue.Flush();
    }

    // Lazily allocate the descriptors
    void AllocateDescriptors();

    void Bind(FrameDescriptorHeap* descriptor_heap);
    void Bind(FrameDescriptorHeap* descriptor_heap, unsigned int frame_idx);

    Texture* CreateTexture(std::wstring file_name);
    RenderTargetTexture* CreateRenderTargetTexture(DXGI_FORMAT format, uint32_t width, uint32_t height);
    DepthMapTexture* CreateDepthTexture(DXGI_FORMAT format, uint32_t width, uint32_t height);


    // Loading all textures to GPU at once
    void Load();

    size_t GetNumTextures() const { return m_srv_texture_map.size() + m_rtv_textures.size() + m_dsv_textures.size(); }

    //void Flush() { m_command_queue.Flush(); }

    void Reset();

    static DescriptorHeap* GetSamplerHeap() { return s_sampler_heap.get(); }
    static D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHeapGpuHandle() { return s_sampler_heap->GetGpuHandle(); }

private:
    // Create static texture samplers
    static std::unique_ptr<DescriptorHeap> CreateSamplers();
};

