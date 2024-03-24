#include "scene.h"

#include "renderer.h"

Scene::Scene() : 
	m_command_queue(CommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)), m_texture_manager(TextureManager(1)), m_num_frame_descriptors(1 + 1) // 1 srv diffuse texture (will be changed every model/mesh) + 1 cbv * framecount
{
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
    m_cbv_srv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Texture* tex = m_texture_manager.Read(L"E:/Rendering/resource/test.dds");

    m_meshes.push_back(Mesh::Read(tex));
}

Scene::~Scene() 
{
	m_command_queue.Flush();
}

void Scene::Init() {
    // Load all textures needs to be done after all descriptors have been allocated
    m_texture_manager.Load();

    // load mesh data from cpu to gpu
    for (auto& mesh : m_meshes) {
        mesh.Load(&m_command_queue);
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

void Scene::CreateSceneBuffer() {
    Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = m_num_frame_descriptors * Renderer::s_num_frames; // 1 srv diffuse texture (will be changed every model/mesh) + 1 cbv * framecount
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ThrowIfFailed(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_cbv_srv_heap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbv_srv_cpu_handle(m_cbv_srv_heap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbv_srv_gpu_handle(m_cbv_srv_heap->GetGPUDescriptorHandleForHeapStart());

    // create a resource heap, descriptor heap, and pointer to cbv for each frame
    for (int i = 0; i < Renderer::s_num_frames; ++i)
    {
        // Ignore the diffuse texture descriptor
        cbv_srv_cpu_handle.Offset(m_cbv_srv_descriptor_size);
        cbv_srv_gpu_handle.Offset(m_cbv_srv_descriptor_size);

        // create cbv
        const UINT constant_buffer_size = (sizeof(SceneConstantBuffer) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto heap_desc = CD3DX12_RESOURCE_DESC::Buffer(constant_buffer_size);
        
        ThrowIfFailed(device->CreateCommittedResource(
            &heap_props, // this heap will be used to upload the constant buffer data
            D3D12_HEAP_FLAG_NONE, // no flags
            &heap_desc, // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
            D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
            nullptr, // we do not have use an optimized clear value for constant buffers
            IID_PPV_ARGS(&m_scene_constant_buffers[i])));
        // Describe and create the scene constant buffer view (CBV) and 
        // cache the GPU descriptor handle.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
        cbv_desc.BufferLocation = m_scene_constant_buffers[i]->GetGPUVirtualAddress();
        cbv_desc.SizeInBytes = constant_buffer_size;    // CB size is required to be 256-byte aligned.
        device->CreateConstantBufferView(&cbv_desc, cbv_srv_cpu_handle);
        m_scene_cbv_handles[i] = cbv_srv_gpu_handle;

        CD3DX12_RANGE read_range(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
        ThrowIfFailed(m_scene_constant_buffers[i]->Map(0, &read_range, reinterpret_cast<void**>(&m_scene_consts_buffer_WO[i])));

        cbv_srv_cpu_handle.Offset(m_cbv_srv_descriptor_size);
        cbv_srv_gpu_handle.Offset(m_cbv_srv_descriptor_size);
    }
}

