#pragma once
#include <expected>
#include <string>
#include <string_view>

struct GLFWwindow;

namespace file_dialog {

auto open(std::string_view filter_name, std::string_view filter_spec, GLFWwindow* parent) -> std::expected<std::string, std::string>;

} // namespace file_dialog
