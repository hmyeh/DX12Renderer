#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>
//#include <DirectXTex.h>

#include <type_traits>
#include <vector>

#include "commandqueue.h"
#include "buffer.h"
#include "texture.h"


// Abstract class? for Mesh
template <IsVertex T>
class IMesh {
protected:
    GpuBuffer<T> m_vertex_buffer;
    GpuBuffer<uint32_t> m_index_buffer;

    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
    D3D12_INDEX_BUFFER_VIEW m_index_buffer_view;

    std::vector<T> m_vertices;
    std::vector<uint32_t> m_indices;

public:
    IMesh() : m_vertex_buffer_view{}, m_index_buffer_view{} {}

    IMesh(const std::vector<T>& verts, const std::vector<uint32_t>& inds) :
        m_vertices(verts), m_indices(inds), m_vertex_buffer_view{}, m_index_buffer_view{}
    {

    }

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertex_buffer_view; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_index_buffer_view; }

    // Load data from CPU -> GPU
    void Load(CommandQueue* command_queue);

    size_t GetNumIndices() const { return m_indices.size(); }

    // maybe bind function for the textures?
};

class ScreenQuad : public IMesh<ScreenVertex> {
private:
    const std::vector<ScreenVertex> vertices {
            { DirectX::XMFLOAT2(-1.0f, -1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) }, // 0
            { DirectX::XMFLOAT2(-1.0f,  1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) }, // 1
            { DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }, // 2
            { DirectX::XMFLOAT2(1.0f, -1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) }, // 3
    };

    const std::vector<uint32_t> indices {
        0, 1, 3, 1, 2, 3
    };

public:
    ScreenQuad() : IMesh<ScreenVertex>(vertices, indices) {}


};

class Skybox : public IMesh<Vertex> {
private:
    // TODO: fix the vertices.... and indices...? https://stackoverflow.com/questions/25482159/normal-vectors-for-an-eight-vertex-cube
    const std::vector<Vertex> vertices{
        { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT3(0.0f,  0.0f, -1.0f) }, // 0
        { DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT2(0.0f, 1.0f), DirectX::XMFLOAT3(0.0f,  0.0f, -1.0f) }, // 1
        { DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f), DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT3(0.0f,  0.0f, -1.0f) }, // 2
        { DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f), DirectX::XMFLOAT2(1.0f, 0.0f), DirectX::XMFLOAT3(0.0f,  0.0f, -1.0f) }, // 3
        { DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT3(0.0f,  0.0f, 1.0f) }, // 4
        { DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT2(0.0f, 1.0f), DirectX::XMFLOAT3(0.0f,  0.0f, 1.0f) }, // 5
        { DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f), DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT3(-1.0f, -1.0, -1.0) }, // 6
        { DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f), DirectX::XMFLOAT2(1.0f, 0.0f), DirectX::XMFLOAT3(-1.0f, -1.0, -1.0) }  // 7
    };

    const std::vector<uint32_t> indices{
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

public:

    Skybox() {}
};

class Mesh : public IMesh<Vertex> {
private:
    Texture* m_diffuse_tex;

public:
    Mesh(const std::vector<Vertex>& verts, const std::vector<uint32_t>& inds, Texture* diffuse_tex) : 
        m_diffuse_tex(diffuse_tex), IMesh<Vertex>(verts, inds)
    {

    }

    void Bind(const D3D12_CPU_DESCRIPTOR_HANDLE& bind_handle) const {
        // only diffuse tex for now
        m_diffuse_tex->Bind(bind_handle);
    }

    // Using https://github.com/tinyobjloader/tinyobjloader
    static Mesh ReadFile(std::string file_name, TextureLibrary* texture_library);
};

