#include "scene.h"

#include "tinyxml2/tinyxml2.h"


#include "buffer.h"
#include "mesh.h"
#include "camera.h"
#include "descriptorheap.h"
#include "utility.h"

Scene::Scene() :
	m_command_queue(CommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)), m_directional_light(&m_texture_library)
{
}

Scene::~Scene() 
{
	m_command_queue.Flush();
}

void Scene::LoadResources() {
    // Load all textures needs to be done after all descriptors have been allocated
    m_texture_library.Load();

    // load mesh data from cpu to gpu
    for (auto& item : m_items) {
        item.mesh.Load(&m_command_queue);
    }

    CreateSceneBuffer();
}

void Scene::Update(unsigned int frame_idx, const Camera& camera) 
{
    //update scene constant buffer // needs to be transposed since row major directxmath and col major hlsl
    m_scene_consts[frame_idx].view = DirectX::XMMatrixTranspose(camera.GetViewMatrix());
    m_scene_consts[frame_idx].projection = DirectX::XMMatrixTranspose(camera.GetProjectionMatrix());
    m_scene_consts[frame_idx].camera_position = camera.GetPosition();

    // update lights
    DirectX::XMFLOAT3 min_bounds, max_bounds;
    ComputeBoundingBox(min_bounds, max_bounds);
    m_directional_light.Update(min_bounds, max_bounds);
    m_scene_consts[frame_idx].directional_light = m_directional_light.GetLightData();

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(m_scene_consts_buffer_WO[frame_idx], &m_scene_consts[frame_idx], sizeof(SceneConstantBuffer));
}

void Scene::Bind(FrameDescriptorHeap* descriptor_heap) 
{
    if (!descriptor_heap->IsShaderVisible())
        throw std::exception("Scene::Bind(): Descriptor heap is not visible to the shaders");

    if (descriptor_heap->GetHeapType() != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        throw std::exception("Scene::Bind(): Incorrect descriptor heap type");

    for (int i = 0; i < Renderer::s_num_frames; ++i)
    {
        // Bind all textures for each frame
        m_texture_library.Bind(descriptor_heap, i);

        // Describe and create the scene constant buffer view (CBV) and cache the GPU descriptor handle
        descriptor_heap->Bind(&m_scene_constant_buffers[i], i);
    }
}

void Scene::ComputeBoundingBox(DirectX::XMFLOAT3& min_bounds, DirectX::XMFLOAT3& max_bounds) 
{
    constexpr float min_fl = std::numeric_limits<float>::min();
    constexpr float max_fl = std::numeric_limits<float>::max();
    DirectX::XMVECTOR scene_min_bounds = DirectX::XMVectorSet(max_fl, max_fl, max_fl, 1.0f);
    DirectX::XMVECTOR scene_max_bounds = DirectX::XMVectorSet(min_fl, min_fl, min_fl, 1.0f);

    for (const auto& item : m_items) {
        DirectX::XMFLOAT4 minb;
        DirectX::XMFLOAT4 maxb;
        item.mesh.GetBounds(minb, maxb);

        // scale rotate translate
        DirectX::XMMATRIX model = item.GetModelMatrix();
        DirectX::XMVECTOR transformed_min_bounds = DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&minb), model);
        DirectX::XMVECTOR transformed_max_bounds = DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&maxb), model);

        // find min/max bounds
        scene_min_bounds = DirectX::XMVectorMin(transformed_min_bounds, scene_min_bounds);
        scene_max_bounds = DirectX::XMVectorMax(transformed_max_bounds, scene_max_bounds);
    }

    DirectX::XMStoreFloat3(&min_bounds, scene_min_bounds);
    DirectX::XMStoreFloat3(&max_bounds, scene_max_bounds);
}

void Scene::CreateSceneBuffer() 
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // create a resource heap, descriptor heap, and pointer to cbv for each frame
    unsigned int constant_buffer_size = (sizeof(SceneConstantBuffer) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
    for (int i = 0; i < Renderer::s_num_frames; ++i)
    {
        // create cbv
        m_scene_constant_buffers[i].Create(constant_buffer_size);

        CD3DX12_RANGE read_range(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
        m_scene_constant_buffers[i].Map(0, &read_range, reinterpret_cast<void**>(&m_scene_consts_buffer_WO[i]));
    }
}


void Scene::ReadXmlFile(const std::string& xml_file)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(xml_file.c_str());
    tinyxml2::XMLElement* scene = doc.FirstChildElement("scene");
    
    if (!scene)
        throw std::exception("XML does not contain scene element");
    
    // Parse Directional Light
    tinyxml2::XMLElement* light = scene->FirstChildElement("light");
    std::string dir_str = light->FirstChildElement("direction")->FirstChild()->Value();
    std::vector<std::string> dir_split = SplitString(dir_str, ",");
    if (dir_split.size() != 3)
        throw std::exception("Incorrect number of directional light elements parsed in XML");
    DirectX::XMFLOAT4 direction(std::stof(dir_split[0]), std::stof(dir_split[1]), std::stof(dir_split[2]), 0.0f);
    m_directional_light.SetDirection(direction);

    std::string color_str = light->FirstChildElement("color")->FirstChild()->Value();
    std::vector<std::string> color_split = SplitString(color_str, ",");
    if (color_split.size() != 4)
        throw std::exception("Incorrect number of color elements parsed in XML");
    DirectX::XMFLOAT4 color(std::stof(color_split[0]), std::stof(color_split[1]), std::stof(color_split[2]), std::stof(color_split[3]));
    m_directional_light.SetColor(color);

    // Parse the scene items
    tinyxml2::XMLElement*  item = scene->FirstChildElement("item");
    while (item) {
        // Create mesh
        std::string mesh_str = item->FirstChildElement("mesh")->FirstChild()->Value();
        Mesh mesh = Mesh::ReadFile(mesh_str, &m_texture_library);

        // Parse position, rotation and scale
        std::string pos_str = item->FirstChildElement("position")->FirstChild()->Value();
        std::vector<std::string> pos_split = SplitString(pos_str, ",");
        if (pos_split.size() != 3)
            throw std::exception("Incorrect number of positional elements parsed in XML");
        DirectX::XMFLOAT4 position (std::stof(pos_split[0]), std::stof(pos_split[1]), std::stof(pos_split[2]), 1.0f);
        
        std::string rot_str = item->FirstChildElement("rotation")->FirstChild()->Value();
        std::vector<std::string> rot_split = SplitString(rot_str, ",");
        if (rot_split.size() != 3)
            throw std::exception("Incorrect number of rotational elements parsed in XML");
        DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(std::stof(rot_split[0]), std::stof(rot_split[1]), std::stof(rot_split[2]));

        std::string scale_str = item->FirstChildElement("scale")->FirstChild()->Value();
        std::vector<std::string> scale_split = SplitString(scale_str, ",");
        if (scale_split.size() != 3)
            throw std::exception("Incorrect number of scaling elements parsed in XML");
        DirectX::XMFLOAT4 scale(std::stof(scale_split[0]), std::stof(scale_split[1]), std::stof(scale_split[2]), 1.0f);

        // Create scene item
        Item scene_item(mesh, position, rotation, scale);
        m_items.push_back(scene_item);

        // Go to next item element
        item = item->NextSiblingElement();
    }

}
