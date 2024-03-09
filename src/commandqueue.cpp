#include "commandqueue.h"

#include "renderer.h"
#include "utility.h"
#include "dx12_api.h"

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type) :
	m_fence_value(0), m_command_list_type(type)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
	m_command_queue = directx::CreateCommandQueue(device, type);

	m_fence = directx::CreateFence(device);
	m_fence_event = directx::CreateEventHandle();

}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::getCommandList()
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list;

	if (!m_command_allocator_queue.empty() && isFenceComplete(m_command_allocator_queue.front().fence_value))
	{
		command_allocator = m_command_allocator_queue.front().command_allocator;
		m_command_allocator_queue.pop();

		ThrowIfFailed(command_allocator->Reset());
	}
	else
	{
		command_allocator = directx::CreateCommandAllocator(device, m_command_list_type);
	}

	if (!m_command_list_queue.empty())
	{
		command_list = m_command_list_queue.front();
		m_command_list_queue.pop();

		ThrowIfFailed(command_list->Reset(command_allocator.Get(), nullptr));
	}
	else
	{
		command_list = directx::CreateCommandList(device, command_allocator, m_command_list_type);
	}

	// Associate the command allocator with the command list so that it can be
	// retrieved when the command list is executed.
	ThrowIfFailed(command_list->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), command_allocator.Get()));

	return command_list;

}

uint64_t CommandQueue::executeCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list)
{
	command_list->Close();

	ID3D12CommandAllocator* command_allocator;
	UINT data_size = sizeof(command_allocator);
	ThrowIfFailed(command_list->GetPrivateData(__uuidof(ID3D12CommandAllocator), &data_size, &command_allocator));

	ID3D12CommandList* const command_lists[] = {
		command_list.Get()
	};

	m_command_queue->ExecuteCommandLists(1, command_lists);
	uint64_t fence_value = signal();

	m_command_allocator_queue.emplace(CommandAllocatorEntry{ fence_value, command_allocator });
	m_command_list_queue.push(command_list);

	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	command_allocator->Release();

	return fence_value;
}

uint64_t CommandQueue::signal()
{
	uint64_t fence_value_for_signal = ++m_fence_value;
	ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), fence_value_for_signal));
	return fence_value_for_signal;
}

void CommandQueue::waitForFenceValue(uint64_t fence_value)
{
	if (m_fence->GetCompletedValue() < fence_value)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence_value, m_fence_event));
		::WaitForSingleObject(m_fence_event, DWORD_MAX);
	}
}


void CommandQueue::flush()
{
	uint64_t fence_value_for_signal = signal();
	waitForFenceValue(fence_value_for_signal);
}