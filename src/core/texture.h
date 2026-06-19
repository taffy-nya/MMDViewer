#pragma once
#include <glad/glad.h>
#include <expected>
#include <string>
#include <filesystem>
#include <unordered_map>

struct TextureInfo {
    std::string path;
    GLuint gl_texture_id{0};
};

class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(Texture&&) noexcept;
    Texture& operator=(Texture&&) noexcept;

    static auto load(const std::filesystem::path& path) -> std::expected<Texture, std::string>;

    void bind(int unit = 0) const;
    GLuint id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    GLuint id_{0};
    int width_{0};
    int height_{0};
};

class TextureCache {
public:
    TextureCache() = default;
    ~TextureCache();
    TextureCache(TextureCache&&) noexcept;
    TextureCache& operator=(TextureCache&&) noexcept;

    auto get_or_load(const std::filesystem::path& path) -> std::expected<const Texture*, std::string>;
    GLuint default_tex();

private:
    std::unordered_map<std::string, Texture> cache_;
    GLuint default_tex_{0};
};
