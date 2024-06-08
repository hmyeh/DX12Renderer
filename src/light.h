#pragma once

#include <DirectXMath.h>

#include "texture.h"

//struct PointLight{
//	DirectX::XMFLOAT4 position;
//	DirectX::XMFLOAT4 color;
//	double radius;
//};

const unsigned int SHADOWMAP_SIZE = 1024;


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
	DirectionalLight(TextureLibrary* texture_library);
	DirectionalLight(TextureLibrary* texture_library, const DirectX::XMFLOAT4& direction, const DirectX::XMFLOAT4& color);


	// Use Bounds of the scene to update the light data
	void Update(const DirectX::XMFLOAT3& min_bounds, const DirectX::XMFLOAT3& max_bounds);

	DepthMapTexture* GetDepthMap() { return m_depthmap; }
	const DirectionalLightData& GetLightData() const { return m_light_data; }

	void SetDirection(const DirectX::XMFLOAT4& direction);
	void SetColor(const DirectX::XMFLOAT4& color) { m_light_data.color = color; }
};

