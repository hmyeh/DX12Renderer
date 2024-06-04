#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_6.h>

#include <array>

#include "buffer.h"

// Interface for rendertargets
class IRenderTarget : public IResourceType {
protected:
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv_handle;
    void CreateRenderTargetView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr);

public:
    static float s_clear_value[4];
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetHandle() const { return m_rtv_handle; }
    virtual void ClearRenderTarget(CommandList& command_list) = 0;
    
    // in case resource has been reset and recreated for resize
    void ResourceChanged(ID3D12Resource* resource);
};

class IDepthStencilTarget : public IResourceType {
protected:
    static const std::array<DXGI_FORMAT, 5> s_valid_formats;
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsv_handle;
    virtual void CreateDepthStencilView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr);

public:
    static const D3D12_DEPTH_STENCIL_VALUE s_clear_value;

    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilHandle() const { return m_dsv_handle; }
    virtual void ClearDepthStencil(CommandList& command_list) = 0;

    // in case resource has been reset and recreated for resize
    void ResourceChanged(ID3D12Resource* resource);

    static bool IsValidFormat(DXGI_FORMAT format) { return std::find(s_valid_formats.begin(), s_valid_formats.end(), format) != s_valid_formats.end(); }


    static DXGI_FORMAT TypelessToShaderFormat(DXGI_FORMAT format) {
        DXGI_FORMAT dsv_format = DXGI_FORMAT_UNKNOWN;

        switch (format) {
        case DXGI_FORMAT_R32_TYPELESS:
            dsv_format = DXGI_FORMAT_R32_FLOAT;
            break;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            dsv_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // ?
            break;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            dsv_format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; // ?
            break;
        case DXGI_FORMAT_R16_TYPELESS:
            dsv_format = DXGI_FORMAT_R16_FLOAT;
            break;
        }

        return dsv_format;
    }

    // Convert shader resource format to depth stencil format
    static DXGI_FORMAT TypelessToDepthStencilFormat(DXGI_FORMAT format) {
        DXGI_FORMAT dsv_format = DXGI_FORMAT_UNKNOWN;

        switch (format) {
        case DXGI_FORMAT_R32_TYPELESS:
            dsv_format = DXGI_FORMAT_D32_FLOAT;
            break;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            dsv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            break;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            dsv_format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            break;
        case DXGI_FORMAT_R16_TYPELESS:
            dsv_format = DXGI_FORMAT_D16_UNORM;
            break;
        }
        
        return dsv_format;
    }
};


class RenderBuffer : public GpuResource, public IRenderTarget {
private:
    uint64_t m_fence_value;

public:
    RenderBuffer() : m_fence_value(0) {}

    void Create(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swap_chain, unsigned int frame_idx);
    uint64_t GetFenceValue() const { return m_fence_value; }
    void SetFenceValue(uint64_t updated_fence_value) {
        //if (updated_fence_value < m_fence_value)
        //    throw std::exception("this should not be happening");
        m_fence_value = updated_fence_value;
    }

    virtual void ClearRenderTarget(CommandList& command_list) override;
    void CreateRenderTargetView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr) { IRenderTarget::CreateRenderTargetView(m_resource.Get(), handle, desc); }

    void Present(CommandList& command_list) { TransitionResourceState(command_list, D3D12_RESOURCE_STATE_PRESENT); }
};

class DepthBuffer : public GpuResource, public IDepthStencilTarget {
private:
    DXGI_FORMAT m_format;

public:
    DepthBuffer() : m_format(DXGI_FORMAT_UNKNOWN), GpuResource() {}

    void Create(DXGI_FORMAT format, uint32_t width, uint32_t height);

    virtual void ClearDepthStencil(CommandList& command_list) override;
    void CreateDepthStencilView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr) { IDepthStencilTarget::CreateDepthStencilView(m_resource.Get(), handle, desc); }

    void Resize(uint32_t width, uint32_t height) {
        Destroy();
        Create(m_format, width, height);
        IDepthStencilTarget::ResourceChanged(m_resource.Get());
    }
};
