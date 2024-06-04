#pragma once

#define NOMINMAX
#include <DirectXMath.h>

#include <vector>
#include <array>

#include "commandqueue.h"
#include "texture.h"
#include "renderer.h"
#include "light.h"


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

        DirectX::XMMATRIX GetModelMatrix() const {
            DirectX::XMVECTOR object_space_origin = DirectX::XMVectorSet(0, 0, 0, 1);
            return DirectX::XMMatrixAffineTransformation(DirectX::XMLoadFloat4(&scale), object_space_origin, DirectX::XMLoadFloat4(&rotation), DirectX::XMLoadFloat4(&position));
        }
    };

private:
    struct SceneConstantBuffer { // TODO: aligning for XMMATRIX instead of XMFLOAT4x4? https://learn.microsoft.com/en-us/cpp/cpp/align-cpp?view=msvc-170&redirectedfrom=MSDN
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
        DirectX::XMFLOAT4 camera_position;
        DirectionalLightData directional_light;
    };

    // note: DirectX matrices are row-major / HLSL matrices are column-major (needs to be transposed) https://sakibsaikia.github.io/graphics/2017/07/06/Matrix-Operations-In-Shaders.html
    SceneConstantBuffer m_scene_consts[Renderer::s_num_frames];
    SceneConstantBuffer* m_scene_consts_buffer_WO[Renderer::s_num_frames]; //WRITE ONLY POINTER
    // resource to store the scene constant buffer
    UploadBuffer m_scene_constant_buffers[Renderer::s_num_frames];
    //D3D12_GPU_DESCRIPTOR_HANDLE m_scene_cbv_handles[Renderer::s_num_frames];

	// Commandqueue for copying
	CommandQueue m_command_queue;

    // Store scene items
    std::vector<Item> m_items;

    // Texturemanager allocates the non-shader visible heap descriptors, texture can then be bound afterwards to copy the descriptor to shader visible heap
    TextureLibrary m_texture_library;

    // Lights
    DirectionalLight m_directional_light;
    D3D12_GPU_DESCRIPTOR_HANDLE m_dir_light_handles[Renderer::s_num_frames];
    //std::vector<PointLight> m_point_lights;

public:
	Scene();
	~Scene(); // TODO: not properly destroying the scene at close


    // create the gpu/upload buffers needed for the 
    void LoadResources();

    // Update the scene constant buffer for the current frame index
    void Update(unsigned int frame_idx, const Camera& camera);

    // Bind the scene constant buffer to a shader visible descriptor heap
    void Bind(FrameDescriptorHeap* descriptor_heap);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSceneConstantsHandle(unsigned int frame_idx) const { return m_scene_constant_buffers[frame_idx].GetBufferGPUHandle(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetDirectionalLightHandle(unsigned int frame_idx) { return m_directional_light.GetDepthMap()->GetShaderGPUHandle(frame_idx); }

    const std::vector<Item>& GetSceneItems() const { return m_items; }
    DepthMapTexture* GetDirectionalLightDepthMap() { return m_directional_light.GetDepthMap(); }

    void ComputeBoundingBox(DirectX::XMFLOAT3& min_bounds, DirectX::XMFLOAT3& max_bounds);

    // get number of descriptors per frame ( + 1 from constant scene buffer)
    unsigned int GetNumFrameDescriptors() const { return m_texture_library.GetNumTextures() + 1; }

    void ReadXmlFile(const std::string& xml_file);

    void Flush() { m_command_queue.Flush(); /*m_texture_library.Flush();*/ }
private:
    void CreateSceneBuffer();
};
