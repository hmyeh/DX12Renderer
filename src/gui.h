#pragma once

#include "Windows.h"

#include "buffer.h"

// Forward declarations
class FrameDescriptorHeap;
class CommandList;

// Dummy class for reserving descriptors 
class ImguiResource : public GpuResource {

};

// Structs to share data with renderer
// GUI writes to the data and renderer only reads the data
struct ImageOptions {
	float gamma;
	float exposure;
	//float pad1;
	//float pad2;

	static unsigned int GetNum32BitValues() { return sizeof(ImageOptions) / 4; }
};

class GUI {
private:
	ImguiResource m_dummy_resource;

	// Variables to manipulate in the GUI
	ImageOptions m_img_options;
	bool m_initialized;

public:
	GUI(HWND hWnd);
	~GUI();

	void Bind(FrameDescriptorHeap* descriptor_heap, DXGI_FORMAT render_target_format);
	
	bool WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void Render(CommandList& command_list);

	unsigned int GetNumResources() const { return 1; }

	//float GetGamma() const { return m_gamma; }
	ImageOptions* GetImageOptions() { return &m_img_options; }
};
