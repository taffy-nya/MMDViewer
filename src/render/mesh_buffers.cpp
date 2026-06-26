#include "render/mesh_buffers.h"
#include <cstddef>

namespace render {

auto MeshBuffers::create(const ModelData& data) -> std::expected<MeshBuffers, std::string> {
    const auto& verts = data.vertices;
    if (verts.empty()) {
        return std::unexpected(std::string("model has no vertices"));
    }

    MeshBuffers mb;

    glGenVertexArrays(1, &mb.vao_);
    glBindVertexArray(mb.vao_);

    glGenBuffers(1, &mb.vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, mb.vbo_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mb.ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mb.ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 data.faces.size() * sizeof(Face),
                 data.faces.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex),
                           reinterpret_cast<const void*>(offsetof(Vertex, bone_indices)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, bone_weights)));
    glEnableVertexAttribArray(4);

    glGenBuffers(1, &mb.bone_tbo_);
    glBindBuffer(GL_TEXTURE_BUFFER, mb.bone_tbo_);
    glGenTextures(1, &mb.bone_tex_);

    mb.index_count_ = static_cast<int>(data.faces.size() * sizeof(Face) / sizeof(unsigned int));

    glBindVertexArray(0);
    return mb;
}

MeshBuffers::~MeshBuffers() {
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
    if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
    if (ebo_ != 0) glDeleteBuffers(1, &ebo_);
    if (bone_tbo_ != 0) glDeleteBuffers(1, &bone_tbo_);
    if (bone_tex_ != 0) glDeleteTextures(1, &bone_tex_);
}

MeshBuffers::MeshBuffers(MeshBuffers&& other) noexcept
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_)
    , bone_tbo_(other.bone_tbo_), bone_tex_(other.bone_tex_)
    , index_count_(other.index_count_) {
    other.vao_ = 0; other.vbo_ = 0; other.ebo_ = 0;
    other.bone_tbo_ = 0; other.bone_tex_ = 0;
    other.index_count_ = 0;
}

MeshBuffers& MeshBuffers::operator=(MeshBuffers&& other) noexcept {
    if (this != &other) {
        if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
        if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
        if (ebo_ != 0) glDeleteBuffers(1, &ebo_);
        if (bone_tbo_ != 0) glDeleteBuffers(1, &bone_tbo_);
        if (bone_tex_ != 0) glDeleteTextures(1, &bone_tex_);
        vao_ = other.vao_; vbo_ = other.vbo_; ebo_ = other.ebo_;
        bone_tbo_ = other.bone_tbo_; bone_tex_ = other.bone_tex_;
        index_count_ = other.index_count_;
        other.vao_ = 0; other.vbo_ = 0; other.ebo_ = 0;
        other.bone_tbo_ = 0; other.bone_tex_ = 0;
        other.index_count_ = 0;
    }
    return *this;
}

void MeshBuffers::update_bone_matrices(const std::vector<glm::mat4>& mats) {
    if (bone_tbo_ == 0 || mats.empty()) return;
    glBindBuffer(GL_TEXTURE_BUFFER, bone_tbo_);
    glBufferData(GL_TEXTURE_BUFFER, mats.size() * sizeof(glm::mat4), mats.data(), GL_DYNAMIC_DRAW);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, bone_tex_);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, bone_tbo_);
}

} // namespace render
