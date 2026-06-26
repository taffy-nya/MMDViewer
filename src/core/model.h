#pragma once
#include <glm/glm.hpp>
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

struct ModelData {
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<Material> materials;
    std::vector<BoneDef> bone_defs;
    std::vector<std::string> tex_paths;
};
