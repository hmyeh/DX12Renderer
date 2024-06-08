#include "light.h"


DirectionalLight::DirectionalLight(TextureLibrary* texture_library) : 
	m_light_data(DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)), m_depthmap(nullptr)
{
	m_depthmap = texture_library->CreateDepthTexture(DXGI_FORMAT_R32_TYPELESS, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
}

DirectionalLight::DirectionalLight(TextureLibrary* texture_library, const DirectX::XMFLOAT4& direction, const DirectX::XMFLOAT4& color) :
	m_light_data(direction, color), m_depthmap(nullptr)
{
	m_depthmap = texture_library->CreateDepthTexture(DXGI_FORMAT_R32_TYPELESS, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
}

void DirectionalLight::Update(const DirectX::XMFLOAT3& min_bounds, const DirectX::XMFLOAT3& max_bounds) {
	using namespace DirectX;

	float eps = 1e-4f;
	if (std::abs(m_light_data.direction.x) < eps && std::abs(m_light_data.direction.z) < eps) {
		// Slightly move the direction vector and normalize
		m_light_data.direction.x += eps;
		XMVECTOR direction_vec = XMVector4Normalize(XMLoadFloat4(&m_light_data.direction));
		XMStoreFloat4(&m_light_data.direction, direction_vec);
	}
	DirectX::XMVECTOR light_dir = XMLoadFloat4(&m_light_data.direction);

	XMVECTOR max_bounds_vec = XMLoadFloat3(&max_bounds);
	XMVECTOR min_bounds_vec = XMLoadFloat3(&min_bounds);
	XMVECTOR bounds_vec = XMVectorAbs(max_bounds_vec - min_bounds_vec);

	// Compute position and target for view matrix
	XMVECTOR target = 0.5f * bounds_vec + min_bounds_vec;
	XMVECTOR position = target - light_dir;
	// XMMatrixLookAtLH does not work straight down or up, due to using cross product on (pos-center) x up (parallel vectors) -> 0
	XMMATRIX light_view = XMMatrixLookAtLH(position, target, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

	// Transform AABB to light view space and recompute bounds
	XMVECTOR transformed_min_bounds = XMVector3Transform(min_bounds_vec, light_view);
	XMVECTOR transformed_max_bounds = XMVector3Transform(max_bounds_vec, light_view);
	XMFLOAT3 proj_max_bounds;
	XMFLOAT3 proj_min_bounds;
	XMStoreFloat3(&proj_max_bounds, XMVectorMax(transformed_min_bounds, transformed_max_bounds));
	XMStoreFloat3(&proj_min_bounds, XMVectorMin(transformed_min_bounds, transformed_max_bounds));

	// Construct the projection matrix with the computed bounds
	float view_width = std::abs(proj_max_bounds.x - proj_min_bounds.x);
	float view_height = std::abs(proj_max_bounds.z - proj_min_bounds.z);
	float near_plane = proj_min_bounds.y;
	float far_plane = proj_max_bounds.y;
	XMMATRIX light_projection = DirectX::XMMatrixOrthographicLH(view_width, view_height, near_plane, far_plane);

	// Send transpose to hlsl shader as transpose matrix
	m_light_data.lightspace_mat = XMMatrixMultiplyTranspose(light_view, light_projection);
}