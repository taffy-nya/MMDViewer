#pragma once
#include <glad/glad.h>
#include <expected>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "render/shader.h"
#include "render/mesh_buffers.h"

class Camera;
struct Light;
struct Model;
struct TextureInfo;

namespace render {

class ModelRenderer {
public:
    static auto create(const Model& model, const std::vector<TextureInfo>& textures)
        -> std::expected<ModelRenderer, std::string>;

    ModelRenderer() = default;
    ~ModelRenderer();
    ModelRenderer(ModelRenderer&& other) noexcept;
    ModelRenderer& operator=(ModelRenderer&& other) noexcept;
    ModelRenderer(const ModelRenderer&) = delete;
    ModelRenderer& operator=(const ModelRenderer&) = delete;

    void draw(const Camera& camera,
              const glm::mat4& model_matrix,
              const std::vector<glm::mat4>& bone_mats,
              const std::vector<Light>& lights,
              const glm::vec3& ambient, float ambient_strength,
              GLuint shadow_map, const glm::mat4& light_space,
              float brightness = 1.0f);

    void draw_shadow(const glm::mat4& model_matrix,
                     const std::vector<glm::mat4>& bone_mats,
                     const glm::mat4& light_space);

    void rebind(const Model& model, const std::vector<TextureInfo>& textures) {
        model_ = &model;
        textures_ = &textures;
    }

private:
    const Model* model_ = nullptr;
    const std::vector<TextureInfo>* textures_ = nullptr;

    Shader main_shader_;
    Shader edge_shader_;
    Shader shadow_shader_;
    MeshBuffers buffers_;

    struct LightLocs {
        GLint position, direction, color, intensity, type;
        GLint constant, linear, quadratic, enabled;
    } light_locs_[16], edge_light_locs_[16];

    void cache_main_uniforms();
    void cache_edge_uniforms();
    void cache_shadow_uniforms();
    void set_lights_uniforms(const LightLocs* locs, const std::vector<Light>& lights, int count);

    GLuint default_toon_tex_ = 0;
};

} // namespace render
