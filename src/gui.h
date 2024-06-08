#pragma once

#include "Windows.h"

#include "buffer.h"
#include "pipeline.h"

// Forward declarations
class FrameDescriptorHeap;
class CommandList;


// Dummy class for reserving descriptors 
class ImguiResource : public GpuResource {

};


class GUI {
private:
	ImguiResource m_dummy_resource;

	// Variables to manipulate in the GUI
	ImagePipeline::Options* m_img_options;
	bool m_initialized;

public:
	GUI(HWND hWnd);
	~GUI();

	void Bind(FrameDescriptorHeap* descriptor_heap, DXGI_FORMAT render_target_format);
	
	bool WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void Render(CommandList& command_list);

	unsigned int GetNumResources() const { return 1; }

	void SetImageOptions(ImagePipeline::Options* options) { m_img_options = options; }
};
