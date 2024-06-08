#include "gui.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include "renderer.h"
#include "descriptorheap.h"
#include "commandlist.h"



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


GUI::GUI(HWND hWnd) : 
	m_img_options(nullptr), m_initialized(false)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform backends
	ImGui_ImplWin32_Init(hWnd); // Move to constructor
}

GUI::~GUI() 
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void GUI::Bind(FrameDescriptorHeap* descriptor_heap, DXGI_FORMAT render_target_format) 
{
	unsigned int bind_idx = descriptor_heap->Bind(&m_dummy_resource);

	// If already initialized before then shutdown and reinitialize
	if (m_initialized)
		ImGui_ImplDX12_Shutdown(); // gave bool initialize

	// Setup Renderer backends
	ImGui_ImplDX12_Init(Renderer::GetDevice().Get(), Renderer::s_num_frames, render_target_format,
		descriptor_heap->GetDescriptorHeap(),
		// You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
		descriptor_heap->GetImguiCpuHandle(bind_idx),
		descriptor_heap->GetImguiGpuHandle(bind_idx));

	m_initialized = true;
}

bool GUI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

void GUI::Render(CommandList& command_list)
{
	// (Your code process and dispatch Win32 messages)
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (m_img_options)
	{
		ImGui::Begin("Options");

		ImGui::Text("Image Effect Options");
		ImGui::SliderFloat("gamma", &m_img_options->gamma, 0.1f, 5.0f);
		ImGui::SliderFloat("exposure", &m_img_options->exposure, 0.1f, 5.0f);

		ImGui::End();
	}

	// Rendering
	// (Your code clears your framebuffer, renders your other stuff etc.)
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list.GetD12CommandList().Get());
	// (Your code calls ExecuteCommandLists, swapchain's Present(), etc.)
}
