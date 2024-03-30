#include "utility.h"

std::wstring to_wstring(std::string str)
{
    // overestimate number of code points
    std::wstring wstr(str.size() + 1, L' ');
    size_t out_size;
    mbstowcs_s(&out_size, &wstr[0], wstr.size(), str.c_str(), wstr.size() - 1);
    // shrink to fit and remove null terminated character https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/mbstowcs-s-mbstowcs-s-l?view=msvc-170
    wstr.resize(out_size - 1);
    return wstr;
}
