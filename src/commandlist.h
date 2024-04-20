#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <d3dx12.h>

#include <vector>

#include "buffer.h"
#include "descriptorheap.h"

class CommandList {
private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_command_list;
	D3D12_COMMAND_LIST_TYPE m_command_list_type;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator;

	std::vector<UploadBuffer> m_upload_buffers;
public:
	CommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator, D3D12_COMMAND_LIST_TYPE command_list_type);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetD12CommandList() const { return m_command_list; }
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetD12CommandAllocator() const { return m_command_allocator; }
	void SetCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator);

	// Use Uploadbuffer to copy data to GPU and keep buffer in-flight during execution of commandlist
	void UploadBufferData(uint64_t upload_size, ID3D12Resource* destination_resource, unsigned int num_subresources, D3D12_SUBRESOURCE_DATA* subresources_data);

	// Pipeline state
	void SetPipelineState(ID3D12PipelineState* pipeline_state) { m_command_list->SetPipelineState(pipeline_state); }

	// Mesh
	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitive_topology) { m_command_list->IASetPrimitiveTopology(primitive_topology); }
	void SetVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& vert_buffer_view) { m_command_list->IASetVertexBuffers(0, 1, &vert_buffer_view); }
	void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ind_buffer_view) { m_command_list->IASetIndexBuffer(&ind_buffer_view); }

	// Root Signature
	void SetGraphicsRootSignature(ID3D12RootSignature* root_signature) { m_command_list->SetGraphicsRootSignature(root_signature); }
	void SetDescriptorHeaps(unsigned int num_heaps, ID3D12DescriptorHeap** heaps) { m_command_list->SetDescriptorHeaps(num_heaps, heaps); }
	void SetGraphicsRootDescriptorTable(unsigned int param_idx, const D3D12_GPU_DESCRIPTOR_HANDLE& descriptor) { m_command_list->SetGraphicsRootDescriptorTable(param_idx, descriptor); }

	// descriptor heap should only be set once
	void SetDescriptorHeaps(std::vector<DescriptorHeap*> heaps);

	// Render target
	void SetViewport(const D3D12_VIEWPORT& viewport) { m_command_list->RSSetViewports(1, &viewport); }
	void SetScissorRect(const D3D12_RECT& scissor_rect) { m_command_list->RSSetScissorRects(1, &scissor_rect); }

	void SetRenderTargets(unsigned int num_rtvs, D3D12_CPU_DESCRIPTOR_HANDLE* render_target_views, D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_view) { m_command_list->OMSetRenderTargets(num_rtvs, render_target_views, FALSE, depth_stencil_view); }
	void ClearRenderTargetView(const D3D12_CPU_DESCRIPTOR_HANDLE& rtv, const float clear_color[4]) { m_command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr); }
	void ClearDepthStencilView(const D3D12_CPU_DESCRIPTOR_HANDLE& dsv, float depth) { m_command_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr); }

	void DrawIndexedInstanced(unsigned int num_indices, unsigned int num_instances) { m_command_list->DrawIndexedInstanced(num_indices, num_instances, 0, 0, 0); }


	void ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after);

	void Close() { m_command_list->Close(); }
};
