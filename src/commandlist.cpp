#include "commandlist.h"

#include "renderer.h"
#include "utility.h"
#include "dx12_api.h"
#include "buffer.h"

CommandList::CommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator, D3D12_COMMAND_LIST_TYPE command_list_type) :
	m_command_allocator(command_allocator), m_command_list_type(command_list_type)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
	m_command_list = directx::CreateCommandList(device, command_allocator, m_command_list_type);
}

void CommandList::SetCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator) 
{
	m_command_allocator = command_allocator;
	ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), nullptr));
	// Clearing previously made upload buffers
	m_upload_buffers.clear();
}

void CommandList::UploadBufferData(uint64_t upload_size, ID3D12Resource* destination_resource, unsigned int num_subresources, D3D12_SUBRESOURCE_DATA* subresources_data) {
	// Create upload buffer of appropriate size
	UploadBuffer upload_buffer;
	upload_buffer.Create(upload_size);
	// Store upload buffer for in-flight uploads
	m_upload_buffers.push_back(upload_buffer);

	UpdateSubresources(m_command_list.Get(), destination_resource, upload_buffer.GetResource(), 0, 0, num_subresources, subresources_data);
}

void CommandList::ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, state_before, state_after);
	m_command_list->ResourceBarrier(1, &barrier);
}
