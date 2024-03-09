#pragma once

#include <DirectXMath.h>

class Camera {
private:

    float m_fov;
    float m_near;
    float m_far;

    float m_aspect_ratio; // TODO: reisze width height

    DirectX::XMVECTOR m_position;
    DirectX::XMVECTOR m_up;

public:
    Camera(uint32_t width, uint32_t height, DirectX::XMVECTOR position = DirectX::XMVectorSet(0, 0, -10, 1), DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0), float fov = 45.0f, float nearZ = 0.1f, float farZ = 100.0f) :
        m_position(position), m_up(up), m_fov(fov), m_near(nearZ), m_far(farZ), m_aspect_ratio(width / static_cast<float>(height))
    {

    }

    DirectX::XMMATRIX GetViewMatrix() const {
        return DirectX::XMMatrixLookAtLH(m_position, DirectX::XMVectorSet(0, 0, 0, 1), m_up);
    }

    DirectX::XMMATRIX GetProjectionMatrix() const {
        return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_fov), m_aspect_ratio, m_near, m_far);
    }
};
