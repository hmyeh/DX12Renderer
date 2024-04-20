#include "scene.h"

#include "renderer.h"

Scene::Scene() :
	m_command_queue(CommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)), m_num_frame_descriptors(1 + 1) // 1 srv diffuse texture (will be changed every model/mesh NO THIS DOES NOT WORK THIS WAY SINCE ALL DRAWS ARE QUEUED UP ON SAME COMMANDLIST...) + 1 cbv * framecount
{
}

Scene::~Scene() 
{
	m_command_queue.Flush();
}

void Scene::LoadResources() {
    // for now set here
    m_items.push_back(Item{ Mesh::ReadFile("resource/box.obj", &m_texture_library) });

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

void Scene::Bind(DescriptorHeap* descriptor_heap) {// Note: could cache the descriptorheap pointer if needed not currently done
    if (!descriptor_heap->IsShaderVisible())
        throw std::exception("The descriptor heap to bind to is not shader visible");

    if (descriptor_heap->GetHeapType() != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        throw std::exception("The descriptor heap is not of the right type");

    m_descriptor_heap = descriptor_heap;

    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    // descriptor heap should have a way to keep track of how many descriptors are needed per frame
    // scene knows how many it needs itself
    for (int i = 0; i < Renderer::s_num_frames; ++i)
    {
        // Maybe also pre-emptively do texture binds here
        for (unsigned int item_idx = 0; item_idx < m_items.size(); ++item_idx) {
            m_items[item_idx].mesh.Bind(descriptor_heap->GetCpuHandle(i * m_num_frame_descriptors + item_idx));// ASSUMING SINGLE TEXTURE PER MESH TODO: FIX ALSO DONT FORGET THAT THE PER FRAME RESOURCES COULD INCLUDE NON-SCENE TEXTURES... FOR EXAMPLE THE PIPELINE IMAGES
            m_items[item_idx].gpu_handle[i] = descriptor_heap->GetGpuHandle(i * m_num_frame_descriptors + item_idx);
        }

        // Describe and create the scene constant buffer view (CBV) and 
        // cache the GPU descriptor handle.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
        cbv_desc.BufferLocation = m_scene_constant_buffers[i]->GetGPUVirtualAddress();
        cbv_desc.SizeInBytes = m_constant_buffer_size;    // CB size is required to be 256-byte aligned.
        device->CreateConstantBufferView(&cbv_desc, descriptor_heap->GetCpuHandle(i * m_num_frame_descriptors + m_items.size()));// ASSUMING SINGLE TEXTURE PER MESH TODO: FIX
        m_scene_cbv_handles[i] = descriptor_heap->GetGpuHandle(i * m_num_frame_descriptors + m_items.size());// ASSUMING SINGLE TEXTURE PER MESH TODO: FIX
    }
}

void Scene::CreateSceneBuffer() {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    // create a resource heap, descriptor heap, and pointer to cbv for each frame
    m_constant_buffer_size = (sizeof(SceneConstantBuffer) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
    for (int i = 0; i < Renderer::s_num_frames; ++i)
    {
        // create cbv
        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto heap_desc = CD3DX12_RESOURCE_DESC::Buffer(m_constant_buffer_size);
        
        ThrowIfFailed(device->CreateCommittedResource(
            &heap_props, // this heap will be used to upload the constant buffer data
            D3D12_HEAP_FLAG_NONE, // no flags
            &heap_desc, // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
            D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
            nullptr, // we do not have use an optimized clear value for constant buffers
            IID_PPV_ARGS(&m_scene_constant_buffers[i])));

        CD3DX12_RANGE read_range(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
        ThrowIfFailed(m_scene_constant_buffers[i]->Map(0, &read_range, reinterpret_cast<void**>(&m_scene_consts_buffer_WO[i])));
    }
}

