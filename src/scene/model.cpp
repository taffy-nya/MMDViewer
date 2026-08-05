#include "scene/model.h"
#include "core/model_loader.h"
#include "core/anim_loader.h"
#include "animation/ik_solver.h"
#include <ranges>
#include <glm/gtc/matrix_transform.hpp>

auto Model::load(const std::string& path) -> std::expected<Model, std::string> {
    Model m;

    auto result = core::load_pmx(path);
    if (!result) return std::unexpected(result.error());
    m.data_ = std::move(*result);

    m.skeleton_.init(m.data_.bone_defs);
    m.morph_ctrl_.init(m.data_);

    auto last_slash = path.rfind('/');
    if (last_slash == std::string::npos) last_slash = path.rfind('\\');
    auto base_path = (last_slash != std::string::npos) ? path.substr(0, last_slash + 1) : "";

    m.textures_.reserve(m.data_.tex_paths.size());
    for (const auto& tex_path : m.data_.tex_paths) {
        TextureInfo info;
        info.path = tex_path;
        if (!tex_path.empty()) {
            std::string full_path = base_path + tex_path;
            for (char& c : full_path) if (c == '\\') c = '/';
            auto tex_result = m.tex_cache_.get_or_load(full_path);
            if (tex_result) info.gl_texture_id = (*tex_result)->id();
        }
        m.textures_.push_back(info);
    }

    auto mr_result = render::ModelRenderer::create(m.data_);
    if (!mr_result) return std::unexpected(mr_result.error());
    m.renderer_ = std::make_unique<render::ModelRenderer>(std::move(*mr_result));

    return m;
}

auto Model::load_motion(const std::string& path) -> std::expected<void, std::string> {
    auto anim_result = core::load_vmd(path);
    if (!anim_result) return std::unexpected(anim_result.error());
    if (data_.bone_defs.empty()) return std::unexpected("No model loaded");
    morph_ctrl_.reset();
    auto result = anim_player_.set_anim(*anim_result, data_.bone_defs, data_.morph_defs);
    if (!result) return std::unexpected(result.error());
    current_frame = 0;
    return {};
}

void Model::update_anim(float dt) {
    if (enable_motion && !manual_bone_control) {
        if (is_playing) {
            current_frame += dt * 30.0f;
            if (current_frame >= anim_player_.duration()) {
                current_frame = 0;
            }
            anim_player_.update(current_frame, data_.bone_defs, skeleton_);
            anim_player_.update_morphs(current_frame, morph_ctrl_);
        }
    } else if (!enable_motion && !manual_bone_control) {
        skeleton_.reset_pose();
    }
}

void Model::update_morphs() {
    morph_ctrl_.resolve(data_);
    morph_ctrl_.apply_bone_morphs(data_, skeleton_.states());
    if (morph_ctrl_.is_vertex_uv_dirty()) {
        morphed_vertices_ = data_.vertices;
        morph_ctrl_.apply_vertex_morphs(data_, morphed_vertices_);
        morph_ctrl_.apply_uv_morphs(data_, morphed_vertices_);
    }
    if (!data_.morph_defs.empty()) {
        morphed_materials_ = data_.materials;
        for (int i = 0; i < static_cast<int>(morphed_materials_.size()); ++i) {
            morph_ctrl_.apply_material(data_, i, morphed_materials_[i]);
        }
    }
}

void Model::update_bones() {
    auto& defs = data_.bone_defs;
    auto& sk   = skeleton_;
    for (int pass = 0; pass < 2; ++pass) {
        sk.update_transforms(defs);
        for (auto&& [idx, def] : std::views::enumerate(defs)) {
            if (def.ik_target_index != -1) {
                IKSolver::solve(static_cast<int>(idx), defs, sk);
            }
        }
    }
}

void Model::reset_pose() {
    skeleton_.reset_pose();
}

void Model::reset_transform() {
    translation = glm::vec3(0);
    rotation    = glm::vec3(0);
    scale       = glm::vec3(1);
}

auto Model::model_matrix() const -> glm::mat4 {
    glm::mat4 m(1.0f);
    m = glm::translate(m, translation);
    m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, scale);
    return m;
}

void Model::draw(const Camera& camera,
                 const std::vector<Light>& lights,
                 const glm::vec3& ambient, float ambient_strength,
                 GLuint shadow_map, const glm::mat4& light_space) {
    if (!renderer_) return;
    auto bone_mats = data_.bone_defs.empty() ? std::vector<glm::mat4>{} : skeleton_.get_bone_matrices();
    const auto* verts = morph_ctrl_.is_vertex_uv_dirty() ? &morphed_vertices_ : nullptr;
    const auto* mats = (!morphed_materials_.empty()) ? &morphed_materials_ : nullptr;
    renderer_->draw(data_, textures_, camera, model_matrix(), bone_mats, lights,
                    ambient, ambient_strength, shadow_map, light_space, 1.0f, verts, mats);
}

void Model::draw_shadow(const glm::mat4& light_space) {
    if (!renderer_) return;
    auto bone_mats = data_.bone_defs.empty() ? std::vector<glm::mat4>{} : skeleton_.get_bone_matrices();
    const auto* verts = morph_ctrl_.is_vertex_uv_dirty() ? &morphed_vertices_ : nullptr;
    renderer_->draw_shadow(data_, model_matrix(), bone_mats, light_space, verts);
}
