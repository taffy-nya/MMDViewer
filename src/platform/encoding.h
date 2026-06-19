#pragma once
#include <cstdio>
#include <string>
#include <string_view>

namespace encoding {

std::wstring to_wide(std::string_view str, unsigned int codepage);

std::string utf16_to_utf8(const wchar_t* wstr, int wlen);

std::string sjis_to_utf8(std::string_view sjis);

std::string ansi_to_utf8(std::string_view ansi);

FILE* fopen_utf8(const char* path, const char* mode);

} // namespace encoding
