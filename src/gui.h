#pragma once

#include "Windows.h"

#include "buffer.h"

// Forward declarations
class FrameDescriptorHeap;
class CommandList;

// Dummy class for reserving descriptors 
class ImguiResource : public GpuResource {

};

class GUI {
private:
	ImguiResource m_dummy_resource;

public:
	GUI();

	~GUI();

	void Init(HWND hWnd, FrameDescriptorHeap* descriptor_heap, DXGI_FORMAT render_target_format);
	
	bool WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void Render(CommandList& command_list);

	unsigned int GetNumResources() const { return 1; }
};
