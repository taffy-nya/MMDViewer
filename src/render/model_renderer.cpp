#include "render/model_renderer.h"
#include "core/camera.h"
#include "core/model.h"
#include "core/texture.h"
#include "core/light.h"
#include <iterator>
#include <format>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"

namespace render {

void ModelRenderer::cache_main_uniforms() {
    for (int i = 0; i < 16; ++i) {
        const std::string base = "lights[" + std::to_string(i) + "]";
        light_locs_[i].position  = main_shader_.uniform(base + ".position");
        light_locs_[i].direction = main_shader_.uniform(base + ".direction");
        light_locs_[i].color     = main_shader_.uniform(base + ".color");
        light_locs_[i].intensity = main_shader_.uniform(base + ".intensity");
        light_locs_[i].type      = main_shader_.uniform(base + ".type");
        light_locs_[i].constant  = main_shader_.uniform(base + ".constant");
        light_locs_[i].linear    = main_shader_.uniform(base + ".linear");
        light_locs_[i].quadratic = main_shader_.uniform(base + ".quadratic");
        light_locs_[i].enabled   = main_shader_.uniform(base + ".enabled");
    }
    main_shader_.uniform("receiveShadow");
}

void ModelRenderer::cache_edge_uniforms() {
    for (int i = 0; i < 16; ++i) {
        const std::string base = "lights[" + std::to_string(i) + "]";
        edge_light_locs_[i].position  = edge_shader_.uniform(base + ".position");
        edge_light_locs_[i].direction = edge_shader_.uniform(base + ".direction");
        edge_light_locs_[i].color     = edge_shader_.uniform(base + ".color");
        edge_light_locs_[i].intensity = edge_shader_.uniform(base + ".intensity");
        edge_light_locs_[i].type      = edge_shader_.uniform(base + ".type");
        edge_light_locs_[i].constant  = edge_shader_.uniform(base + ".constant");
        edge_light_locs_[i].linear    = edge_shader_.uniform(base + ".linear");
        edge_light_locs_[i].quadratic = edge_shader_.uniform(base + ".quadratic");
        edge_light_locs_[i].enabled   = edge_shader_.uniform(base + ".enabled");
    }
}

void ModelRenderer::cache_shadow_uniforms() {
    shadow_shader_.uniform("model");
    shadow_shader_.uniform("lightSpaceMatrix");
    shadow_shader_.uniform("boneMatrices");
}

void ModelRenderer::set_lights_uniforms(std::span<const LightLocs> locs, const std::vector<Light>& lights) {
    for (int i = 0; i < std::ssize(lights) && i < std::ssize(locs); ++i) {
        const auto& light = lights[i];
        const auto& l = locs[i];
        glUniform3fv(l.position,  1, glm::value_ptr(light.position));
        glUniform3fv(l.direction, 1, glm::value_ptr(light.direction));
        glUniform3fv(l.color,     1, glm::value_ptr(light.color));
        glUniform1f(l.intensity,  light.intensity);
        glUniform1i(l.type,       light.type);
        glUniform1f(l.constant,   light.constant);
        glUniform1f(l.linear,     light.linear);
        glUniform1f(l.quadratic,  light.quadratic);
        glUniform1i(l.enabled,    light.enabled);
    }
}

auto ModelRenderer::create(const ModelData& data)
    -> std::expected<ModelRenderer, std::string> {
    ModelRenderer r;

    auto buf_result = MeshBuffers::create(data);
    if (!buf_result) return std::unexpected(buf_result.error());
    r.buffers_ = std::move(*buf_result);

    auto main_result = Shader::from_paths("shaders/vshader.glsl", "shaders/fshader_with_light.glsl");
    if (!main_result) return std::unexpected(main_result.error());
    r.main_shader_ = std::move(*main_result);
    r.cache_main_uniforms();

    auto edge_result = Shader::from_paths("shaders/vshader_edge.glsl", "shaders/fshader_edge.glsl");
    if (!edge_result) return std::unexpected(edge_result.error());
    r.edge_shader_ = std::move(*edge_result);
    r.cache_edge_uniforms();

    auto shadow_result = Shader::from_paths("shaders/vshader_shadow.glsl", "shaders/fshader_shadow.glsl");
    if (!shadow_result) return std::unexpected(shadow_result.error());
    r.shadow_shader_ = std::move(*shadow_result);
    r.cache_shadow_uniforms();

    glGenTextures(1, &r.default_toon_tex_);
    glBindTexture(GL_TEXTURE_2D, r.default_toon_tex_);
    unsigned char default_toon[3] = { 255, 255, 255 }; 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, default_toon);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    r.shared_toon_texs_.resize(k_shared_toon_count, 0);
    stbi_set_flip_vertically_on_load(true);
    for (int i = 0; i < k_shared_toon_count; ++i) {
        std::string path = std::format("toons/toon{:02d}.bmp", i + 1);
        int w, h, ch;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
        if (data) {
            GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
            glGenTextures(1, &r.shared_toon_texs_[i]);
            glBindTexture(GL_TEXTURE_2D, r.shared_toon_texs_[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(fmt), w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            stbi_image_free(data);
        }
    }

    return r;
}

ModelRenderer::~ModelRenderer() {
    if (default_toon_tex_ != 0) glDeleteTextures(1, &default_toon_tex_);
    for (auto t : shared_toon_texs_) {
        if (t != 0) glDeleteTextures(1, &t);
    }
}

ModelRenderer::ModelRenderer(ModelRenderer&& other) noexcept
    : main_shader_(std::move(other.main_shader_))
    , edge_shader_(std::move(other.edge_shader_))
    , shadow_shader_(std::move(other.shadow_shader_))
    , buffers_(std::move(other.buffers_))
    , default_toon_tex_(other.default_toon_tex_)
    , shared_toon_texs_(std::move(other.shared_toon_texs_)) {
    for (int i = 0; i < 16; ++i) {
        light_locs_[i] = other.light_locs_[i];
        edge_light_locs_[i] = other.edge_light_locs_[i];
    }
    other.default_toon_tex_ = 0;
}

ModelRenderer& ModelRenderer::operator=(ModelRenderer&& other) noexcept {
    if (this != &other) {
        if (default_toon_tex_ != 0) glDeleteTextures(1, &default_toon_tex_);
        for (auto t : shared_toon_texs_) {
            if (t != 0) glDeleteTextures(1, &t);
        }
        main_shader_ = std::move(other.main_shader_);
        edge_shader_ = std::move(other.edge_shader_);
        shadow_shader_ = std::move(other.shadow_shader_);
        buffers_ = std::move(other.buffers_);
        default_toon_tex_ = other.default_toon_tex_;
        shared_toon_texs_ = std::move(other.shared_toon_texs_);
        for (int i = 0; i < 16; ++i) {
            light_locs_[i] = other.light_locs_[i];
            edge_light_locs_[i] = other.edge_light_locs_[i];
        }
        other.default_toon_tex_ = 0;
    }
    return *this;
}

void ModelRenderer::draw(const ModelData& data,
                         const std::vector<TextureInfo>& textures,
                         const Camera& camera,
                         const glm::mat4& model_matrix,
                         const std::vector<glm::mat4>& bone_mats,
                         const std::vector<Light>& lights,
                         const glm::vec3& ambient, float ambient_strength,
                         GLuint shadow_map, const glm::mat4& light_space,
                         float brightness,
                         const std::vector<Vertex>* morphed_vertices,
                         const std::vector<Material>* morphed_materials) {
    glm::mat4 view = camera.get_view_matrix();
    glm::mat4 proj = camera.get_projection_matrix();
    glm::vec3 view_pos = camera.position;

    buffers_.bind();
    if (morphed_vertices) {
        buffers_.update_vertices(*morphed_vertices);
    }
    if (!bone_mats.empty()) {
        buffers_.update_bone_matrices(bone_mats);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    edge_shader_.use();

    edge_shader_.set_int("boneMatrices", 2);
    edge_shader_.set_mat4("model", model_matrix);
    edge_shader_.set_mat4("view", view);
    edge_shader_.set_mat4("projection", proj);
    edge_shader_.set_vec3("ambientColor", ambient);
    edge_shader_.set_float("ambientStrength", ambient_strength);
    edge_shader_.set_int("numLights", static_cast<int>(lights.size()));
    set_lights_uniforms(edge_light_locs_, lights);

    const auto& edge_mats = (morphed_materials && !morphed_materials->empty())
        ? *morphed_materials : data.materials;

    unsigned int face_offset = 0;
    for (const auto& mat : edge_mats) {
        if (!(mat.draw_flags & 0x10)) {   // 0x10: エッジ有効 (绘制描边)
            face_offset += mat.face_count;
            continue;
        }
        edge_shader_.set_float("edge_size", mat.edge_size * 0.03f);
        edge_shader_.set_vec4("edge_color", mat.edge_color);
        glDrawElements(GL_TRIANGLES, mat.face_count, GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(static_cast<uintptr_t>(face_offset * sizeof(unsigned int))));
        face_offset += mat.face_count;
    }

    glCullFace(GL_BACK);
    main_shader_.use();

    main_shader_.set_int("boneMatrices", 2);
    main_shader_.set_int("shadowMap", 10);
    main_shader_.set_mat4("lightSpaceMatrix", light_space);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, shadow_map);

    main_shader_.set_mat4("model", model_matrix);
    main_shader_.set_mat4("view", view);
    main_shader_.set_mat4("projection", proj);
    main_shader_.set_vec3("viewPos", view_pos);
    main_shader_.set_vec3("ambientColor", ambient);
    main_shader_.set_float("ambientStrength", ambient_strength);
    main_shader_.set_int("numLights", static_cast<int>(lights.size()));
    main_shader_.set_float("brightness", brightness);
    set_lights_uniforms(light_locs_, lights);

    main_shader_.set_int("textureSampler", 0);
    main_shader_.set_int("toonSampler", 1);

    const auto& main_mats = (morphed_materials && !morphed_materials->empty())
        ? *morphed_materials : data.materials;

    face_offset = 0;
    for (const auto& mat : main_mats) {
        main_shader_.set_int("receiveShadow", (mat.draw_flags & 0x08) ? 1 : 0); // 0x08: セルフシャドウ (receive shadow)
        main_shader_.set_vec4("objectColor", mat.diffuse);
        main_shader_.set_float("shininess", mat.shininess);
        main_shader_.set_vec3("specularColor", mat.specular);

        if (mat.has_texture()) {
            main_shader_.set_int("hasTexture", 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[mat.tex_idx].gl_texture_id);
        } else {
            main_shader_.set_int("hasTexture", 0);
        }

        glActiveTexture(GL_TEXTURE1);
        if (mat.use_shared_toon) {
            int idx = mat.toon_tex_idx;
            if (idx >= 0 && idx < k_shared_toon_count && shared_toon_texs_[idx] != 0)
                glBindTexture(GL_TEXTURE_2D, shared_toon_texs_[idx]);
            else
                glBindTexture(GL_TEXTURE_2D, default_toon_tex_);
        } else if (mat.toon_tex_idx >= 0 && mat.toon_tex_idx < static_cast<int>(textures.size())) {
            glBindTexture(GL_TEXTURE_2D, textures[mat.toon_tex_idx].gl_texture_id);
        } else {
            glBindTexture(GL_TEXTURE_2D, default_toon_tex_);
        }

        if (mat.draw_flags & 0x01) {    // 0x01: 両面描画 (双面渲染)
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }

        glDrawElements(GL_TRIANGLES, mat.face_count, GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(static_cast<uintptr_t>(face_offset * sizeof(unsigned int))));
        face_offset += mat.face_count;
    }

    glDisable(GL_CULL_FACE);
    glBindVertexArray(0);
}

void ModelRenderer::draw_shadow(const ModelData& data,
                                const glm::mat4& model_matrix,
                                const std::vector<glm::mat4>& bone_mats,
                                const glm::mat4& light_space,
                                const std::vector<Vertex>* morphed_vertices) {
    shadow_shader_.use();
    shadow_shader_.set_mat4("model", model_matrix);
    shadow_shader_.set_mat4("lightSpaceMatrix", light_space);

    if (morphed_vertices) {
        buffers_.update_vertices(*morphed_vertices);
    }
    buffers_.update_bone_matrices(bone_mats);
    shadow_shader_.set_int("boneMatrices", 2);

    buffers_.bind();

    unsigned int face_offset = 0;
    for (const auto& mat : data.materials) {
        if (!(mat.draw_flags & 0x04)) {   // 0x04: セルフシャドウマップ (产生阴影)
            face_offset += mat.face_count;
            continue;
        }
        if (mat.draw_flags & 0x01) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        glDrawElements(GL_TRIANGLES, mat.face_count, GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(static_cast<uintptr_t>(face_offset * sizeof(unsigned int))));
        face_offset += mat.face_count;
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindVertexArray(0);
}

} // namespace render
