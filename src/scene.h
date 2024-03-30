#pragma once

#include <vector>

#include "mesh.h"
#include "commandqueue.h"
#include "camera.h"
#include "texture.h"
#include "renderer.h"
//class Renderer;

class Scene {
private:
    struct SceneConstantBuffer {
        DirectX::XMMATRIX model; // global scene world model
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    };
    // note: DirectX matrices are row-major / HLSL matrices are column-major (needs to be transposed) https://sakibsaikia.github.io/graphics/2017/07/06/Matrix-Operations-In-Shaders.html
    SceneConstantBuffer m_scene_consts[Renderer::s_num_frames];
    SceneConstantBuffer* m_scene_consts_buffer_WO[Renderer::s_num_frames]; //WRITE ONLY POINTER
    // resource to store the scene constant buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> m_scene_constant_buffers[Renderer::s_num_frames];
    D3D12_GPU_DESCRIPTOR_HANDLE m_scene_cbv_handles[Renderer::s_num_frames];
    unsigned int m_cbv_srv_descriptor_size;
    unsigned int m_num_frame_descriptors;

	// Commandqueue for copying
	CommandQueue m_command_queue;

	std::vector<Mesh> m_meshes;

    // Single heap for descriptors of CBV SRV type over multiple frames
    // Heap description: 1 srv (diffuse) texture, 1 cbv (scene constant buffer), next frame....
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbv_srv_heap;

    // Texturemanager allocates the non-shader visible heap descriptors, texture can then be bound afterwards to copy the descriptor to shader visible heap
    TextureLibrary m_texture_library;
public:
	Scene();
	~Scene();


    // create the gpu/upload buffers needed for the 
    void Load();

    // Update the scene constant buffer for the current frame index
    void Update(unsigned int frame_idx, const Camera& camera);

    // Bind scene item for rendering
    void BindSceneItem(unsigned int frame_idx, const Mesh& mesh) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbv_srv_cpu_handle(m_cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(), frame_idx * m_num_frame_descriptors * m_cbv_srv_descriptor_size);
        // order of descriptor heap is diffuse tex, constant buffer...
        mesh.Bind(cbv_srv_cpu_handle);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetDiffuseTextureHandle(unsigned int frame_idx) { return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbv_srv_heap->GetGPUDescriptorHandleForHeapStart(), frame_idx * m_num_frame_descriptors * m_cbv_srv_descriptor_size); }

    ID3D12DescriptorHeap* GetCbvSrvHeap() { return m_cbv_srv_heap.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSceneConstantsHandle(unsigned int frame_idx) const {  return m_scene_cbv_handles[frame_idx]; }

    ID3D12DescriptorHeap* GetTextureSamplers() { return m_texture_library.GetSamplerHeap(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSamplersHandle() const { return m_texture_library.GetSamplerHeapGpuHandle(); }

    const std::vector<Mesh>& GetItems() const { return m_meshes; }

private:

    void CreateSceneBuffer();
};
