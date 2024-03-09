#pragma once

// DirectX 12 specific headers.
#include <d3d12.h>
#include <wrl.h>

#include <queue>

#if defined(max)
#undef max
#endif


class CommandQueue {
public:
	// Keep track of command allocators that are "in-flight"
	struct CommandAllocatorEntry
	{
		uint64_t fence_value;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
	};

	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue = std::queue< Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> >;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
	D3D12_COMMAND_LIST_TYPE m_command_list_type;

	// Synchronization objects
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fence_value = 0;
	HANDLE m_fence_event;

	CommandAllocatorQueue m_command_allocator_queue;
	CommandListQueue m_command_list_queue;

	CommandQueue(D3D12_COMMAND_LIST_TYPE type);

	virtual ~CommandQueue() {}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> getCommandList();

	uint64_t executeCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list);
	
	uint64_t signal();
	bool isFenceComplete(uint64_t fence_value) { return m_fence->GetCompletedValue() >= fence_value; }

	void waitForFenceValue(uint64_t fence_value);


	void flush();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> getD12CommandQueue() const { return m_command_queue; }
};
