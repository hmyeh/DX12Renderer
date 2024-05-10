#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include <DirectXMath.h>
#include <cstdio>

#include <algorithm> 
#include <cctype>
#include <locale>
#include <string>
#include <stdlib.h>
#include <vector>

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

std::wstring to_wstring(std::string str);

std::vector<std::string> SplitString(std::string str, const std::string& delimiter);

// trim from start (in place)
inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

inline unsigned int CastToUint(size_t size) {
    if (size > UINT_MAX)
        throw std::exception("Failed to cast size_t to unsigned int due to size_t > UINT_MAX");
    return static_cast<unsigned int>(size);
}

// print 4x4 matrix (note: XMMATRIX is stored row-major)
inline void PrintMatrix(DirectX::XMMATRIX mat) 
{
    DirectX::XMFLOAT4X4 mat_view;
    DirectX::XMStoreFloat4x4(&mat_view, mat);
    wchar_t buffer[500];
    swprintf_s(buffer, 500, L"%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f", 
        mat_view._11, mat_view._12, mat_view._13, mat_view._14, 
        mat_view._21, mat_view._22, mat_view._23, mat_view._24, 
        mat_view._31, mat_view._32, mat_view._33, mat_view._34, 
        mat_view._41, mat_view._42, mat_view._43, mat_view._44);
    OutputDebugString(buffer);
}

