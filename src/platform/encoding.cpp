#include "encoding.h"
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

std::wstring encoding::to_wide(std::string_view str, unsigned int codepage) {
    if (str.empty()) return {};
    int len = MultiByteToWideChar(codepage, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring result(len, L'\0');
    MultiByteToWideChar(codepage, 0, str.data(), static_cast<int>(str.size()), result.data(), len);
    return result;
}

std::string encoding::utf16_to_utf8(const wchar_t* wstr, int wlen) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, result.data(), len, nullptr, nullptr);
    return result;
}

std::string encoding::sjis_to_utf8(std::string_view sjis) {
    if (sjis.empty()) return {};
    int w_len = MultiByteToWideChar(932, 0, sjis.data(), static_cast<int>(sjis.size()), nullptr, 0);
    if (w_len <= 0) return std::string(sjis);
    std::vector<wchar_t> w_buf(w_len);
    MultiByteToWideChar(932, 0, sjis.data(), static_cast<int>(sjis.size()), w_buf.data(), w_len);
    return utf16_to_utf8(w_buf.data(), w_len);
}

std::string encoding::ansi_to_utf8(std::string_view ansi) {
    if (ansi.empty()) return {};
    auto w = to_wide(ansi, CP_ACP);
    return utf16_to_utf8(w.data(), static_cast<int>(w.size()));
}

FILE* encoding::fopen_utf8(const char* path, const char* mode) {
    auto wpath = to_wide(path, CP_UTF8);
    auto wmode = to_wide(mode, CP_UTF8);
    return _wfopen(wpath.c_str(), wmode.c_str());
}
