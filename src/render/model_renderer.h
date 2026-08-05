#pragma once
#include <glad/glad.h>
#include <expected>
#include <span>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "render/shader.h"
#include "render/mesh_buffers.h"

class Camera;
struct Light;
struct ModelData;
struct TextureInfo;

namespace render {

class ModelRenderer {
public:
    static auto create(const ModelData& data)
        -> std::expected<ModelRenderer, std::string>;

    ModelRenderer() = default;
    ~ModelRenderer();
    ModelRenderer(ModelRenderer&& other) noexcept;
    ModelRenderer& operator=(ModelRenderer&& other) noexcept;
    ModelRenderer(const ModelRenderer&) = delete;
    ModelRenderer& operator=(const ModelRenderer&) = delete;

    void draw(const ModelData& data,
              const std::vector<TextureInfo>& textures,
              const Camera& camera,
              const glm::mat4& model_matrix,
              const std::vector<glm::mat4>& bone_mats,
              const std::vector<Light>& lights,
              const glm::vec3& ambient, float ambient_strength,
              GLuint shadow_map, const glm::mat4& light_space,
              float brightness = 1.0f,
              const std::vector<Vertex>* morphed_vertices = nullptr,
              const std::vector<Material>* morphed_materials = nullptr);

    void draw_shadow(const ModelData& data,
                     const glm::mat4& model_matrix,
                     const std::vector<glm::mat4>& bone_mats,
                     const glm::mat4& light_space,
                     const std::vector<Vertex>* morphed_vertices = nullptr);

private:
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
    void set_lights_uniforms(std::span<const LightLocs> locs, const std::vector<Light>& lights);

    GLuint default_toon_tex_ = 0;
    std::vector<GLuint> shared_toon_texs_;
    static constexpr int k_shared_toon_count = 10;
};

} // namespace render
