#include "scene.h"

#include "renderer.h"

Scene::Scene() : 
	m_command_queue(CommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)) 
{
	// hardcoded
	m_meshes.push_back(Mesh::Read());
}

Scene::~Scene() 
{
	m_command_queue.flush();
}


void Scene::Load() 
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
	// load mesh from cpu to gpu
	for (auto& mesh : m_meshes) {
		mesh.Load(&m_command_queue);
	}
}