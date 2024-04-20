#pragma once

#include <vector>

#include "mesh.h"
#include "commandqueue.h"
#include "camera.h"
#include "texture.h"
#include "renderer.h"
#include "descriptorheap.h"


// Scene stores the per frame resources/descriptors cached
class Scene {
public:
    struct Item {
        Mesh mesh;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle[Renderer::s_num_frames];

        void Bind() {

        }
    };

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
    //unsigned int m_cbv_srv_descriptor_size;
    unsigned int m_num_frame_descriptors;

    unsigned int m_constant_buffer_size;

	// Commandqueue for copying
	CommandQueue m_command_queue;

    // Store scene items
    std::vector<Item> m_items;

    // Cache bound descriptor heap
    DescriptorHeap* m_descriptor_heap;

    // Texturemanager allocates the non-shader visible heap descriptors, texture can then be bound afterwards to copy the descriptor to shader visible heap
    TextureLibrary m_texture_library;
public:
	Scene();
	~Scene();


    // create the gpu/upload buffers needed for the 
    void LoadResources();

    // Update the scene constant buffer for the current frame index
    void Update(unsigned int frame_idx, const Camera& camera);

    // Bind the scene constant buffer to a shader visible descriptor heap
    void Bind(DescriptorHeap* descriptor_heap);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSceneConstantsHandle(unsigned int frame_idx) const {  return m_scene_cbv_handles[frame_idx]; }

    //ID3D12DescriptorHeap* GetTextureSamplers() { return m_texture_library.GetSamplerHeap(); }
    //D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSamplersHandle() const { return m_texture_library.GetSamplerHeapGpuHandle(); }

    const std::vector<Item>& GetSceneItems() const { return m_items; }

    // get number of descriptors per frame
    unsigned int GetNumFrameDescriptors() const { return m_num_frame_descriptors; }

private:
    void CreateSceneBuffer();
};
