#pragma once
#include <glad/glad.h>
#include <expected>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "core/model.h"

namespace render {

class MeshBuffers {
public:
    static auto create(const ModelData& data) -> std::expected<MeshBuffers, std::string>;

    MeshBuffers() = default;
    ~MeshBuffers();
    MeshBuffers(MeshBuffers&& other) noexcept;
    MeshBuffers& operator=(MeshBuffers&& other) noexcept;
    MeshBuffers(const MeshBuffers&) = delete;
    MeshBuffers& operator=(const MeshBuffers&) = delete;

    void bind() const { glBindVertexArray(vao_); }
    void update_bone_matrices(const std::vector<glm::mat4>& mats);
    int index_count() const { return index_count_; }

private:
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLuint bone_tbo_ = 0, bone_tex_ = 0;
    int index_count_ = 0;
};

} // namespace render
