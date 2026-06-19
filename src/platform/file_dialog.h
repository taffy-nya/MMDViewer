#pragma once
#include <expected>
#include <string>

struct GLFWwindow;

namespace file_dialog {

auto open(const char* filter_name, const char* filter_spec, GLFWwindow* parent) -> std::expected<std::string, std::string>;

} // namespace file_dialog
