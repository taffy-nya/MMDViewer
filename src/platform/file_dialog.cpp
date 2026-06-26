#include "file_dialog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <vector>

static auto ansi_to_utf8(const std::string& ansi) -> std::string {
    if (ansi.empty()) return "";
    int w_len = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, nullptr, 0);
    if (w_len <= 0) return ansi;
    std::vector<wchar_t> w_buf(w_len);
    MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, w_buf.data(), w_len);
    int u8_len = WideCharToMultiByte(CP_UTF8, 0, w_buf.data(), -1, nullptr, 0, nullptr, nullptr);
    if (u8_len <= 0) return ansi;
    std::vector<char> u8_buf(u8_len);
    WideCharToMultiByte(CP_UTF8, 0, w_buf.data(), -1, u8_buf.data(), u8_len, nullptr, nullptr);
    return std::string(u8_buf.data());
}

auto file_dialog::open(std::string_view filter_name, std::string_view filter_spec, GLFWwindow* /*parent*/) -> std::expected<std::string, std::string> {
    std::string filter = std::string(filter_name) + '\0' + std::string(filter_spec) + '\0';
    std::string all = std::string("All Files") + '\0' + "*.*" + '\0';
    filter += all + '\0';

    char buf[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return ansi_to_utf8(std::string(buf));
    }
    if (CommDlgExtendedError() != 0) {
        return std::unexpected("file dialog failed");
    }
    return std::string{};  // user cancelled
}
