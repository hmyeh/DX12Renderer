#include "scene.h"

#include "tinyxml2/tinyxml2.h"

#include "buffer.h"
#include "mesh.h"
#include "camera.h"
#include "descriptorheap.h"
#include "utility.h"

Scene::Scene() :
	m_command_queue(CommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)), m_num_frame_descriptors(1 + 1) // 1 srv diffuse texture (will be changed every model/mesh NO THIS DOES NOT WORK THIS WAY SINCE ALL DRAWS ARE QUEUED UP ON SAME COMMANDLIST...) + 1 cbv * framecount
{
}

Scene::~Scene() 
{
	m_command_queue.Flush();
}

void Scene::LoadResources() {
    //Item item(Mesh::ReadFile("resource/box.obj", &m_texture_library));
    //// for now set here
    //m_items.push_back(item);// Item(Mesh::ReadFile("resource/box.obj", &m_texture_library)));

    // Load all textures needs to be done after all descriptors have been allocated
    m_texture_library.Load();

    // load mesh data from cpu to gpu
    for (auto& item : m_items) {
        item.mesh.Load(&m_command_queue);
    }

    //CreateSamplers();
    CreateSceneBuffer();
}

void Scene::Update(unsigned int frame_idx, const Camera& camera) {
    //update scene constant buffer // needs to be transposed since row major directxmath and col major hlsl
    m_scene_consts[frame_idx].model = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    m_scene_consts[frame_idx].view = DirectX::XMMatrixTranspose(camera.GetViewMatrix());
    m_scene_consts[frame_idx].projection = DirectX::XMMatrixTranspose(camera.GetProjectionMatrix());

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(m_scene_consts_buffer_WO[frame_idx], &m_scene_consts[frame_idx], sizeof(SceneConstantBuffer));
}

void Scene::Bind(FrameDescriptorHeap* descriptor_heap) {// Note: could cache the descriptorheap pointer if needed not currently done
    if (!descriptor_heap->IsShaderVisible())
        throw std::exception("The descriptor heap to bind to is not shader visible");

    if (descriptor_heap->GetHeapType() != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        throw std::exception("The descriptor heap is not of the right type");

    //m_descriptor_heap = descriptor_heap; // Maybe unnecessary????!!
    for (int i = 0; i < Renderer::s_num_frames; ++i)
    {
        // Bind all textures TODO: every frame...
        m_texture_library.Bind(descriptor_heap, i);

        // TODO: make sure meshes get handles to each texture so it doesnt need to keep accessing the map for it...
        for (auto& scene_item : m_items) {
            for (const auto& tex : scene_item.mesh.GetTextures()) {
                scene_item.resource_handles[i].push_back(descriptor_heap->FindResourceHandle(tex, i));
            }
        }

        // Describe and create the scene constant buffer view (CBV) and 
        // cache the GPU descriptor handle.
        unsigned int bind_idx = descriptor_heap->Bind(&m_scene_constant_buffers[i], i);
        m_scene_cbv_handles[i] = descriptor_heap->GetGpuHandle(bind_idx);
    }
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
