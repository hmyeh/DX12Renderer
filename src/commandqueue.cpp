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

CommandList CommandQueue::GetCommandList()
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;

	if (!m_command_allocator_queue.empty() && IsFenceComplete(m_command_allocator_queue.front().fence_value))
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
		CommandList command_list = m_command_list_queue.front();
		m_command_list_queue.pop();

		command_list.SetCommandAllocator(command_allocator);
		return command_list;
	}

	return CommandList(command_allocator, m_command_list_type);
}

uint64_t CommandQueue::ExecuteCommandList(CommandList& command_list)
{
	command_list.Close();

	ID3D12CommandList* const command_lists[] = {
		command_list.GetD12CommandList().Get()
	};

	m_command_queue->ExecuteCommandLists(1, command_lists);
	uint64_t fence_value = Signal();

	m_command_allocator_queue.emplace(CommandAllocatorEntry{ fence_value, command_list.GetD12CommandAllocator().Get() });
	m_command_list_queue.push(command_list);

	return fence_value;
}

uint64_t CommandQueue::Signal()
{
	uint64_t fence_value_for_signal = ++m_fence_value;
	ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), fence_value_for_signal));
	return fence_value_for_signal;
}

void CommandQueue::WaitForFenceValue(uint64_t fence_value)
{
	if (m_fence->GetCompletedValue() < fence_value)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence_value, m_fence_event));
		::WaitForSingleObject(m_fence_event, DWORD_MAX);
	}
}


void CommandQueue::Flush()
{
	uint64_t fence_value_for_signal = Signal();
	WaitForFenceValue(fence_value_for_signal);
}
