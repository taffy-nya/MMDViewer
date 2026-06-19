#include "texture.h"
#include "platform/encoding.h"
#include <format>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <cstdio>

Texture::~Texture() {
    if (id_ != 0) glDeleteTextures(1, &id_);
}

Texture::Texture(Texture&& other) noexcept
    : id_(other.id_), width_(other.width_), height_(other.height_) {
    other.id_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (id_ != 0) glDeleteTextures(1, &id_);
        id_ = other.id_;
        width_ = other.width_;
        height_ = other.height_;
        other.id_ = 0;
    }
    return *this;
}

auto Texture::load(const std::filesystem::path& path) -> std::expected<Texture, std::string> {
    stbi_set_flip_vertically_on_load(true);

    auto* f = encoding::fopen_utf8(path.string().c_str(), "rb");
    if (!f) {
        return std::unexpected(std::format("cannot open texture: {}", path.string()));
    }

    int w, h, ch;
    auto* data = stbi_load_from_file(f, &w, &h, &ch, 0);
    fclose(f);

    if (!data) {
        return std::unexpected(std::format("stb_image failed: {}", stbi_failure_reason()));
    }

    GLenum fmt = GL_RGB;
    if (ch == 1) fmt = GL_RED;
    else if (ch == 4) fmt = GL_RGBA;

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(fmt), w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    Texture tex;
    tex.id_ = id;
    tex.width_ = w;
    tex.height_ = h;
    return tex;
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

TextureCache::~TextureCache() {
    if (default_tex_ != 0) glDeleteTextures(1, &default_tex_);
    cache_.clear();
}

TextureCache::TextureCache(TextureCache&&) noexcept = default;
auto TextureCache::operator=(TextureCache&&) noexcept -> TextureCache& = default;

auto TextureCache::get_or_load(const std::filesystem::path& path) -> std::expected<const Texture*, std::string> {
    auto key = path.string();
    if (auto it = cache_.find(key); it != cache_.end()) {
        return &it->second;
    }
    auto tex = Texture::load(path);
    if (!tex) return std::unexpected(tex.error());
    auto [it, _] = cache_.emplace(key, std::move(*tex));
    return &it->second;
}

GLuint TextureCache::default_tex() {
    if (default_tex_ == 0) {
        glGenTextures(1, &default_tex_);
        glBindTexture(GL_TEXTURE_2D, default_tex_);
        unsigned char white[3] = {255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    return default_tex_;
}
