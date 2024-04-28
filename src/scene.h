#pragma once

#include <vector>

#include "commandqueue.h"
#include "texture.h"
#include "renderer.h"

// Forward declaration
class Mesh;
class Camera;
class UploadBuffer;
class FrameDescriptorHeap;

// Scene stores the per frame resources/descriptors cached
class Scene {
public:
    struct Item {
        Mesh mesh;
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> resource_handles[Renderer::s_num_frames];
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
    UploadBuffer m_scene_constant_buffers[Renderer::s_num_frames];
    D3D12_GPU_DESCRIPTOR_HANDLE m_scene_cbv_handles[Renderer::s_num_frames];
    unsigned int m_num_frame_descriptors;

    //unsigned int m_constant_buffer_size;

	// Commandqueue for copying
	CommandQueue m_command_queue;

    // Store scene items
    std::vector<Item> m_items;

    // Cache bound descriptor heap
    //FrameDescriptorHeap* m_descriptor_heap;

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
    void Bind(FrameDescriptorHeap* descriptor_heap);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSceneConstantsHandle(unsigned int frame_idx) const {  return m_scene_cbv_handles[frame_idx]; }

    const std::vector<Item>& GetSceneItems() const { return m_items; }

    // get number of descriptors per frame
    unsigned int GetNumFrameDescriptors() const { return m_num_frame_descriptors; }

private:
    void CreateSceneBuffer();
};
