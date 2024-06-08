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

// Abstract class for Mesh
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
    // TODO: fix the vertices and indices https://stackoverflow.com/questions/25482159/normal-vectors-for-an-eight-vertex-cube
    static const std::vector<Vertex> vertices;
    static const std::vector<uint32_t> indices;

public:

    Skybox() {}
};

// Store material parameters for shader 
struct MaterialParams {
    float metallic;
    float roughness;
};

class Mesh : public IMesh<Vertex> {
private:
    std::vector<Texture*> m_textures;
    MaterialParams m_mat_params;

    // Cache bounds of Mesh
    DirectX::XMFLOAT4 m_min_bounds; 
    DirectX::XMFLOAT4 m_max_bounds;

public:
    Mesh(const std::vector<Vertex>& verts, const std::vector<uint32_t>& inds, const std::vector<Texture*>& textures) :
        m_textures(textures), m_mat_params{0.0f, 0.25f}, IMesh<Vertex>(verts, inds)
    {
        ComputeBounds();
    }

    std::vector<Texture*> GetTextures() { return m_textures; }//{ m_diffuse_tex }; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetDiffuseTextureDescriptor(unsigned int frame_idx) const;
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetTextureDescriptors(unsigned int frame_idx) const;
    const MaterialParams* GetMaterial() const { return &m_mat_params; }

    void GetBounds(DirectX::XMFLOAT4& min_bounds, DirectX::XMFLOAT4& max_bounds) const { min_bounds = m_min_bounds; max_bounds = m_max_bounds; }

    // Using https://github.com/tinyobjloader/tinyobjloader
    static Mesh ReadFile(std::string file_name, TextureLibrary* texture_library);

private:
    void ComputeBounds();
};

