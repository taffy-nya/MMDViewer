#pragma once
#include <glad/glad.h>
#include <expected>
#include <string>

namespace render {

class ShadowMap {
public:
    static auto create(int width = 2048, int height = 2048)
        -> std::expected<ShadowMap, std::string>;

    ShadowMap() = default;
    ~ShadowMap();
    ShadowMap(ShadowMap&& other) noexcept;
    ShadowMap& operator=(ShadowMap&& other) noexcept;
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void bind_write() const;
    void bind_read(int unit = 10) const;

    GLuint texture() const { return depth_tex_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    GLuint fbo_ = 0;
    GLuint depth_tex_ = 0;
    int width_ = 0, height_ = 0;
};

} // namespace render
