#pragma once

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

std::wstring CastToWString(std::string str);

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

