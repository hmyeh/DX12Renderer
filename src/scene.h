#pragma once

#include <vector>

#include "mesh.h"
#include "commandqueue.h"

class Scene {
private:
	// Commandqueue for copying
	CommandQueue m_command_queue;

	std::vector<Mesh> m_meshes;

public:
	Scene();
	~Scene();

	// Load to gpu
	void Load();

	const std::vector<Mesh>& GetItems() const { return m_meshes; }
};
