#include "render/shadow_map.h"

namespace render {

auto ShadowMap::create(int width, int height) -> std::expected<ShadowMap, std::string> {
    ShadowMap sm;

    glGenFramebuffers(1, &sm.fbo_);

    glGenTextures(1, &sm.depth_tex_);
    glBindTexture(GL_TEXTURE_2D, sm.depth_tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

    glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sm.depth_tex_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    sm.width_ = width;
    sm.height_ = height;
    return sm;
}

ShadowMap::~ShadowMap() {
    if (fbo_ != 0) glDeleteFramebuffers(1, &fbo_);
    if (depth_tex_ != 0) glDeleteTextures(1, &depth_tex_);
}

ShadowMap::ShadowMap(ShadowMap&& other) noexcept
    : fbo_(other.fbo_), depth_tex_(other.depth_tex_)
    , width_(other.width_), height_(other.height_) {
    other.fbo_ = 0;
    other.depth_tex_ = 0;
    other.width_ = 0;
    other.height_ = 0;
}

ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept {
    if (this != &other) {
        if (fbo_ != 0) glDeleteFramebuffers(1, &fbo_);
        if (depth_tex_ != 0) glDeleteTextures(1, &depth_tex_);
        fbo_ = other.fbo_; depth_tex_ = other.depth_tex_;
        width_ = other.width_; height_ = other.height_;
        other.fbo_ = 0; other.depth_tex_ = 0;
        other.width_ = 0; other.height_ = 0;
    }
    return *this;
}

void ShadowMap::bind_write() const {
    glViewport(0, 0, width_, height_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::bind_read(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, depth_tex_);
}

} // namespace render
