#include "mesh.h"

#include <d3dx12.h>

// for tinyobjloader
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <filesystem>

#include "utility.h"
#include "renderer.h"

// Declare the IMesh types to be used to avoid Linker errors https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class IMesh<ScreenVertex>;
template class IMesh<Vertex>;

// Mesh
template<IsVertex T>
void IMesh<T>::Load(CommandQueue* command_queue) {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    auto command_list = command_queue->GetCommandList();

    m_vertex_buffer.Create(m_vertices.size());
    m_vertex_buffer.Upload(command_list, m_vertices);
    m_vertex_buffer_view = m_vertex_buffer.GetVertexBufferView();

    m_index_buffer.Create(m_indices.size());
    m_index_buffer.Upload(command_list, m_indices);
    m_index_buffer_view = m_index_buffer.GetIndexBufferView();

    auto fence_value = command_queue->ExecuteCommandList(command_list);
    command_queue->WaitForFenceValue(fence_value);
}


Mesh Mesh::ReadFile(std::string file_name, TextureLibrary* texture_library) {
    std::filesystem::path file_path(file_name);
    if (file_path.extension() != ".obj")
        throw std::exception("Only accepts .OBJ files");

    if (!std::filesystem::exists(file_path))
        throw std::exception("Obj File not found");

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "resource/"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(file_name, reader_config)) {
        throw std::exception(reader.Error().c_str());
    }

    //if (!reader.Warning().empty()) {
    //    std::cout << "TinyObjReader: " << reader.Warning();
    //}

    // tinyobj storage
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    //Create texture for materials (only using first material)
    Texture* diffuse_tex = texture_library->CreateTexture(to_wstring(materials[0].diffuse_texname));

    // All vertices and indices of the different shapes are combined
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    for (size_t s = 0; s < shapes.size(); s++) {

        for (const tinyobj::index_t& idx : shapes[s].mesh.indices) {
            indices.push_back(idx.vertex_index);

            DirectX::XMFLOAT3 pos(attrib.vertices[3 * size_t(idx.vertex_index)], attrib.vertices[3 * size_t(idx.vertex_index) + 1], attrib.vertices[3 * size_t(idx.vertex_index) + 2]);
            DirectX::XMFLOAT2 tex_coord{};
            // Check if `texcoord_index` is zero or positive. negative = no texcoord data
            if (idx.texcoord_index >= 0)
                tex_coord = DirectX::XMFLOAT2(attrib.texcoords[2 * size_t(idx.texcoord_index)], attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);

            DirectX::XMFLOAT3 normal{};
            if (idx.normal_index >= 0)
                normal = DirectX::XMFLOAT3(attrib.normals[3 * size_t(idx.normal_index)], attrib.normals[3 * size_t(idx.normal_index) + 1], attrib.normals[3 * size_t(idx.normal_index) + 2]);

            vertices.push_back(Vertex{ pos, tex_coord, normal });
        }
    }

    return Mesh(vertices, indices, diffuse_tex);
}
