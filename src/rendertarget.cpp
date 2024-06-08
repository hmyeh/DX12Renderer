#include "rendertarget.h"

#include "utility.h"
#include "renderer.h"
#include "commandlist.h"

// Interface RenderTarget DepthStencil variables
float IRenderTarget::s_clear_value[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
const D3D12_DEPTH_STENCIL_VALUE IDepthStencilTarget::s_clear_value = { 1.0f, 0 };
const std::array<DXGI_FORMAT, 5> IDepthStencilTarget::s_valid_formats = { DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT };

// RenderTarget
void IRenderTarget::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    device->CreateRenderTargetView(resource, desc, handle);
    m_rtv_handle = handle;
}

void IRenderTarget::ResourceChanged(ID3D12Resource* resource)
{
    CreateRenderTargetView(resource, m_rtv_handle);
}

// DepthStencilView
void IDepthStencilTarget::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, D3D12_DEPTH_STENCIL_VIEW_DESC* desc)
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    device->CreateDepthStencilView(resource, desc, handle);
    m_dsv_handle = handle;
}

void IDepthStencilTarget::ResourceChanged(ID3D12Resource* resource)
{
    CreateDepthStencilView(resource, m_dsv_handle);
}


// RenderBuffer
void RenderBuffer::Create(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swap_chain, unsigned int frame_idx) 
{
    if (frame_idx > Renderer::s_num_frames)
        throw std::exception("RenderBuffer::Create(): Out of frame index range");

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    ThrowIfFailed(swap_chain->GetBuffer(frame_idx, IID_PPV_ARGS(&m_resource)));
    // Set to default resource state
    m_resource_state = D3D12_RESOURCE_STATE_PRESENT;
    m_resource->SetName(L"Render Buffer");

    // Specifically due to how render buffer needs to be resized
    if (m_rtv_handle.ptr) // Check if it was already bound
        IRenderTarget::ResourceChanged(m_resource.Get());
}


void RenderBuffer::ClearRenderTarget(CommandList& command_list) {
    // Clearing the buffer means set state to rendertarget
    TransitionResourceState(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);
    command_list.ClearRenderTargetView(m_rtv_handle, IRenderTarget::s_clear_value);
}

// DepthBuffer
void DepthBuffer::Create(DXGI_FORMAT format, uint32_t width, uint32_t height)
{
    if (!IsValidFormat(format))
        throw std::exception("DepthBuffer::Create(): Invalid depth stencil format");

    // Create a depth buffer.
    D3D12_CLEAR_VALUE optimized_clear_value = {};
    optimized_clear_value.Format = format;
    optimized_clear_value.DepthStencil = IDepthStencilTarget::s_clear_value;

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    ThrowIfFailed(device->CreateCommittedResource(
        &heap_props,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimized_clear_value,
        IID_PPV_ARGS(&m_resource)
    ));

    // Update resource state
    m_resource_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    m_format = format;
    m_resource->SetName(L"Depth Stencil Buffer");
}

void DepthBuffer::ClearDepthStencil(CommandList& command_list) 
{
    command_list.ClearDepthStencilView(m_dsv_handle, IDepthStencilTarget::s_clear_value.Depth);
}
