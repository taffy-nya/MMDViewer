#pragma once
#include <expected>
#include <string>
#include <filesystem>
#include "anim.h"

namespace core {

auto load_vmd(const std::filesystem::path& path) -> std::expected<Animation, std::string>;

} // namespace core
