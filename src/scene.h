#pragma once

#include <DirectXMath.h>

#include <vector>
#include <array>

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
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT4 rotation; // quaternion
        DirectX::XMFLOAT4 scale;
        std::array<std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>, Renderer::s_num_frames> resource_handles;// [Renderer::s_num_frames] ;

        Item(const Mesh& mesh, const DirectX::XMFLOAT4& position = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), const DirectX::XMVECTOR& rotation = DirectX::XMQuaternionIdentity(), const DirectX::XMFLOAT4& scale = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)) :
            mesh(mesh), position(position), scale(scale) 
        {
            DirectX::XMStoreFloat4(&this->rotation, rotation);
        }
    };

private:
    struct SceneConstantBuffer { // TODO: aligning for XMMATRIX instead of XMFLOAT4x4? https://learn.microsoft.com/en-us/cpp/cpp/align-cpp?view=msvc-170&redirectedfrom=MSDN
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

    void ReadXmlFile(const std::string& xml_file);

private:
    void CreateSceneBuffer();
};
