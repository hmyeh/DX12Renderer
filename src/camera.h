#pragma once

#include <DirectXMath.h>

class Camera {
private:

    float m_fov;
    float m_near;
    float m_far;

    float m_aspect_ratio; // TODO: reisze width height

    DirectX::XMFLOAT4 m_position;
    DirectX::XMFLOAT4 m_up;

public:
    Camera(uint32_t width, uint32_t height, DirectX::XMFLOAT4 position = DirectX::XMFLOAT4(0.0f, 2.0f, -10.0f, 0.0f), DirectX::XMFLOAT4 up = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f), float fov = 45.0f, float nearZ = 0.1f, float farZ = 100.0f) :
        m_position(position), m_up(up), m_fov(fov), m_near(nearZ), m_far(farZ), m_aspect_ratio(width / static_cast<float>(height))
    {

    }

    DirectX::XMMATRIX GetViewMatrix() const;

    DirectX::XMMATRIX GetProjectionMatrix() const;

    DirectX::XMFLOAT4 GetPosition() const { return m_position; }
};
