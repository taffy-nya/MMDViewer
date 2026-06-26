#pragma once
#include <expected>
#include <string>
#include <filesystem>
#include "model.h"

namespace core {

auto load_pmx(const std::filesystem::path& path) -> std::expected<ModelData, std::string>;

} // namespace core
