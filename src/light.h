#pragma once

#include <DirectXMath.h>

#include "texture.h"

//struct PointLight{
//	DirectX::XMFLOAT4 position;
//	DirectX::XMFLOAT4 color;
//	double radius;
//};


struct DirectionalLightData {
	DirectX::XMFLOAT4 direction;
	DirectX::XMFLOAT4 color;
	DirectX::XMMATRIX lightspace_mat;

	DirectionalLightData() : direction{}, color{}, lightspace_mat{} {}
	DirectionalLightData(const DirectX::XMFLOAT4& direction, const DirectX::XMFLOAT4& color) : direction(direction), color(color), lightspace_mat{} {

	}
};

class DirectionalLight {
private:
	DirectionalLightData m_light_data;
	DepthMapTexture* m_depthmap;

public:
	DirectionalLight(TextureLibrary* texture_library) : m_light_data(DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)) {
		m_depthmap = texture_library->CreateDepthTexture(DXGI_FORMAT_R32_TYPELESS, 1024, 1024); //DXGI_FORMAT_D32_FLOAT 
	}
	

	// Bounds of the scene
	void Update(const DirectX::XMFLOAT3& min_bounds, const DirectX::XMFLOAT3& max_bounds) {
		using namespace DirectX;
		float x_bound = std::max(std::abs(min_bounds.x), max_bounds.x);
		float y_bound = std::max(std::abs(min_bounds.y), max_bounds.y);
		float z_bound = std::max(std::abs(min_bounds.z), max_bounds.z);

		float near_plane = 1.0f;
		float far_plane = 20.0f;//z_bound;
		DirectX::XMMATRIX light_projection = DirectX::XMMatrixOrthographicLH(20.0f, 20.0f, near_plane, far_plane);//2.0f * x_bound, 2.0f * y_bound, near_plane, far_plane);

		// Slightly move 
		float eps = 1e-4;
		if (std::abs(m_light_data.direction.x) < eps && std::abs(m_light_data.direction.z) < eps) {
			//light_dir = DirectX::XMVector4Normalize(light_dir);
			//DirectX::XMVectorSetX()
			m_light_data.direction.x += eps;
		}
		DirectX::XMVECTOR light_dir = DirectX::XMLoadFloat4(&m_light_data.direction);

		//DirectX::XMVECTOR target = 0.5f * DirectX::XMVectorSet(x_bound, y_bound, z_bound, 0.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		//DirectX::XMVECTOR position = target - light_dir * z_bound * 0.5f;
		DirectX::XMVECTOR position = target - light_dir * 10.0f;

		// NOTE!!!:: XMMatrixLookAtLH does not work straight down or up, due to using cross product on (pos-center) x up (parallel vectors) -> 0
		//DirectX::XMMATRIX light_view = DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.0f, 2.0f, -0.1f, 0.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		DirectX::XMMATRIX light_view = DirectX::XMMatrixLookAtLH(position, target, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		// Send transpose to hlsl shader
		//m_light_data.lightspace_mat = DirectX::XMMatrixMultiplyTranspose(light_projection, light_view);
		m_light_data.lightspace_mat = DirectX::XMMatrixMultiplyTranspose(light_view, light_projection);
	}

	DepthMapTexture* GetDepthMap() { return m_depthmap; }
	const DirectionalLightData& GetLightData() const { return m_light_data; }
};

