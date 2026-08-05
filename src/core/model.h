#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <cstdint>

struct IKLink {
    int bone_index{-1};
    bool has_limits{false};
    glm::vec3 min_limit{0};
    glm::vec3 max_limit{0};
};

struct BoneDef {
    std::string name;
    glm::vec3 position{0};
    int parent_index{-1};
    int layer{0};
    unsigned short flags{0};

    int inherit_parent_index{-1};
    float inherit_influence{0.0f};

    int ik_target_index{-1};
    int ik_loop_count{0};
    float ik_limit_angle{0.0f};
    std::vector<IKLink> ik_links;
};

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
    glm::ivec4 bone_indices{0};
    glm::vec4 bone_weights{0.0f};
};

struct Face {
    uint32_t i0{0}, i1{0}, i2{0};
};

struct Material {
    std::string name;
    glm::vec4 diffuse{1};
    glm::vec3 specular{0};
    float shininess{10.0f};
    glm::vec3 ambient{0};

    glm::vec4 edge_color{0, 0, 0, 1};
    float edge_size{1.0f};

    int tex_idx{-1};
    int toon_tex_idx{-1};
    bool use_shared_toon{false};

    uint8_t draw_flags{0};
    uint32_t face_count{0};

    bool has_texture() const { return tex_idx != -1; }
};

enum class MorphType : uint8_t {
    Group = 0, Vertex = 1, Bone = 2, UV = 3,
    AdditionalUV1 = 4, AdditionalUV2 = 5, AdditionalUV3 = 6, AdditionalUV4 = 7,
    Material = 8, Flip = 9, Impulse = 10
};

struct VertexMorphOffset {
    int vertex_index{-1};
    glm::vec3 position_delta{0};
};

struct BoneMorphOffset {
    int bone_index{-1};
    glm::vec3 translation_delta{0};
    glm::vec4 rotation_quat{0, 0, 0, 0};
};

struct UVMorphOffset {
    int vertex_index{-1};
    glm::vec4 uv_delta{0};
};

struct MaterialMorphOffset {
    int material_index{-1};
    uint8_t calc_mode{0};
    glm::vec4 diffuse{1};
    glm::vec3 specular{1};
    float shininess{1};
    glm::vec3 ambient{1};
    glm::vec4 edge_color{1};
    float edge_size{0};
    glm::vec4 tex_coeff{1};
    glm::vec4 sphere_coeff{1};
    glm::vec4 toon_coeff{1};
};

struct GroupMorphOffset {
    int morph_index{-1};
    float weight_ratio{0};
};

struct MorphDef {
    std::string name;
    std::string name_en;
    uint8_t panel{0};
    MorphType type{MorphType::Vertex};

    std::vector<VertexMorphOffset> vertex_offsets;
    std::vector<BoneMorphOffset> bone_offsets;
    std::vector<UVMorphOffset> uv_offsets;
    std::vector<MaterialMorphOffset> material_offsets;
    std::vector<GroupMorphOffset> group_offsets;
};

struct ModelData {
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<Material> materials;
    std::vector<BoneDef> bone_defs;
    std::vector<std::string> tex_paths;
    std::vector<MorphDef> morph_defs;
};
