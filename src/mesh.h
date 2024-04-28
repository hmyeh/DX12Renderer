#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

#include <type_traits>
#include <vector>
#include <string>

#include "buffer.h"


// Forward declaration
class CommandQueue;
class Texture;
class TextureLibrary;

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
    static const std::vector<ScreenVertex> vertices;
    static const std::vector<uint32_t> indices;

public:
    ScreenQuad() : IMesh<ScreenVertex>(vertices, indices) {}


};

class Skybox : public IMesh<Vertex> {
private:
    // TODO: fix the vertices.... and indices...? https://stackoverflow.com/questions/25482159/normal-vectors-for-an-eight-vertex-cube
    static const std::vector<Vertex> vertices;
    static const std::vector<uint32_t> indices;

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

    std::vector<Texture*> GetTextures() { return { m_diffuse_tex }; }

    // Using https://github.com/tinyobjloader/tinyobjloader
    static Mesh ReadFile(std::string file_name, TextureLibrary* texture_library);
};

