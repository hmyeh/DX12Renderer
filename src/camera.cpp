#include "camera.h"


DirectX::XMMATRIX Camera::GetViewMatrix() const {
    DirectX::XMFLOAT4 target(0.0f, 0.0f, 0.0f, 0.0f);
    return DirectX::XMMatrixLookAtLH(XMLoadFloat4(&m_position), XMLoadFloat4(&target), XMLoadFloat4(&m_up));
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const {
    return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_fov), m_aspect_ratio, m_near, m_far);
}