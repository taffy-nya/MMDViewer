#include "model_loader.h"
#include "platform/encoding.h"
#include <fstream>
#include <format>
#include <vector>
#include <cstring>

namespace core {

template <typename T>
static void read_data(std::ifstream& file, T& val) {
    file.read(reinterpret_cast<char*>(&val), sizeof(T));
}

static int read_index(std::ifstream& file, int size) {
    unsigned int index = 0;
    char buffer[4];
    file.read(buffer, size);
    if (!file) return -1;
    switch (size) {
        case 1: index = *reinterpret_cast<unsigned char*>(buffer); break;
        case 2: index = *reinterpret_cast<unsigned short*>(buffer); break;
        case 4: index = *reinterpret_cast<unsigned int*>(buffer); break;
    }
    if ((size == 1 && index == 255) || (size == 2 && index == 65535) || (size == 4 && index == 4294967295))
        return -1;
    return static_cast<int>(index);
}

static std::string read_pmx_str(std::ifstream& file, int text_encoding) {
    int length;
    read_data(file, length);
    if (!file || length <= 0) return "";
    std::vector<char> buffer(length);
    file.read(buffer.data(), length);
    if (!file) return "";
    if (text_encoding == 1) {
        return std::string(buffer.begin(), buffer.end());
    } else {
        auto* w_str = reinterpret_cast<const wchar_t*>(buffer.data());
        int w_len = length / static_cast<int>(sizeof(wchar_t));
        return encoding::utf16_to_utf8(w_str, w_len);
    }
}

static void skip_pmx_str(std::ifstream& file) {
    int length;
    read_data(file, length);
    if (file && length > 0) file.seekg(length, std::ios_base::cur);
}

auto load_pmx(const std::filesystem::path& path) -> std::expected<ModelData, std::string> {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(std::format("cannot open PMX: {}", path.string()));
    }

    char magic[4];
    file.read(magic, 4);
    if (strncmp(magic, "PMX ", 4) != 0) {
        return std::unexpected("not a valid PMX file");
    }

    float version;
    read_data(file, version);

    unsigned char global_info_size;
    read_data(file, global_info_size);
    unsigned char g_info[8];
    file.read(reinterpret_cast<char*>(g_info), global_info_size);
    unsigned char text_encoding = g_info[0];
    unsigned char uv_size = g_info[1];
    unsigned char vertex_index_size = g_info[2];
    unsigned char texture_index_size = g_info[3];
    [[maybe_unused]] unsigned char material_index_size = g_info[4];
    unsigned char bone_index_size = g_info[5];
    [[maybe_unused]] unsigned char morph_index_size = g_info[6];
    [[maybe_unused]] unsigned char rigid_body_index_size = g_info[7];

    // Skip model names
    skip_pmx_str(file);
    skip_pmx_str(file);
    skip_pmx_str(file);
    skip_pmx_str(file);

    ModelData data;

    // Read vertices
    int num_vertices;
    read_data(file, num_vertices);
    data.vertices.reserve(num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        Vertex v;
        read_data(file, v.position);
        read_data(file, v.normal);
        read_data(file, v.uv);
        // MMD left-handed → OpenGL right-handed: flip Z, flip UV.y
        v.position.z = -v.position.z;
        v.normal.z = -v.normal.z;
        v.uv.y = 1.0f - v.uv.y;
        if (uv_size > 0) {
            file.seekg(16 * uv_size, std::ios::cur);
        }

        unsigned char weight_type;
        read_data(file, weight_type);
        switch (weight_type) {
            case 0: { // BDEF1
                v.bone_indices[0] = read_index(file, bone_index_size);
                v.bone_weights[0] = 1.0f;
                break;
            }
            case 1: { // BDEF2
                v.bone_indices[0] = read_index(file, bone_index_size);
                v.bone_indices[1] = read_index(file, bone_index_size);
                float w1;
                read_data(file, w1);
                v.bone_weights[0] = w1;
                v.bone_weights[1] = 1.0f - w1;
                break;
            }
            case 2: { // BDEF4
                for (auto& idx : {&v.bone_indices[0], &v.bone_indices[1], &v.bone_indices[2], &v.bone_indices[3]})
                    *idx = read_index(file, bone_index_size);
                read_data(file, v.bone_weights[0]);
                read_data(file, v.bone_weights[1]);
                read_data(file, v.bone_weights[2]);
                read_data(file, v.bone_weights[3]);
                break;
            }
            case 3: { // SDEF (simplified to BDEF2)
                v.bone_indices[0] = read_index(file, bone_index_size);
                v.bone_indices[1] = read_index(file, bone_index_size);
                float w1;
                read_data(file, w1);
                file.seekg(36, std::ios::cur); // skip C, R0, R1
                v.bone_weights[0] = w1;
                v.bone_weights[1] = 1.0f - w1;
                break;
            }
            case 4: { // QDEF (simplified to BDEF4)
                for (auto& idx : {&v.bone_indices[0], &v.bone_indices[1], &v.bone_indices[2], &v.bone_indices[3]})
                    *idx = read_index(file, bone_index_size);
                read_data(file, v.bone_weights[0]);
                read_data(file, v.bone_weights[1]);
                read_data(file, v.bone_weights[2]);
                read_data(file, v.bone_weights[3]);
                break;
            }
        }
        data.vertices.push_back(v);
        file.seekg(4, std::ios::cur);   // skip edge scale
    }

    // Read faces
    int num_face_indices;
    read_data(file, num_face_indices);
    data.faces.reserve(num_face_indices / 3);
    for (int i = 0; i < num_face_indices / 3; ++i) {
        auto v1 = static_cast<uint32_t>(read_index(file, vertex_index_size));
        auto v2 = static_cast<uint32_t>(read_index(file, vertex_index_size));
        auto v3 = static_cast<uint32_t>(read_index(file, vertex_index_size));
        data.faces.emplace_back(v1, v3, v2); // swap v2/v3 for handedness
    }

    // Read textures
    int num_textures;
    read_data(file, num_textures);
    data.tex_paths.reserve(num_textures);
    for (int i = 0; i < num_textures; ++i) {
        data.tex_paths.push_back(read_pmx_str(file, text_encoding));
    }

    // Read materials
    int num_materials;
    read_data(file, num_materials);
    data.materials.reserve(num_materials);
    for (int i = 0; i < num_materials; ++i) {
        Material mat;
        skip_pmx_str(file);
        skip_pmx_str(file);
        read_data(file, mat.diffuse);
        read_data(file, mat.specular);
        read_data(file, mat.shininess);
        read_data(file, mat.ambient);
        unsigned char draw_flags;
        read_data(file, draw_flags);
        mat.draw_flags = draw_flags;
        read_data(file, mat.edge_color);
        read_data(file, mat.edge_size);
        mat.tex_idx = read_index(file, texture_index_size);
        file.seekg(texture_index_size + 1, std::ios::cur);
        unsigned char toon_ref_type;
        read_data(file, toon_ref_type);
        if (toon_ref_type == 0) {
            mat.toon_tex_idx = read_index(file, texture_index_size);
            mat.use_shared_toon = false;
        } else {
            unsigned char internal_index;
            read_data(file, internal_index);
            mat.toon_tex_idx = internal_index;
            mat.use_shared_toon = true;
        }
        skip_pmx_str(file);
        read_data(file, mat.face_count);
        data.materials.push_back(mat);
    }

    // Read bones
    int num_bones;
    read_data(file, num_bones);
    data.bone_defs.reserve(num_bones);
    for (int i = 0; i < num_bones; ++i) {
        BoneDef b;
        b.name = read_pmx_str(file, text_encoding);
        skip_pmx_str(file); // English name
        read_data(file, b.position);
        b.position.z = -b.position.z;
        b.parent_index = read_index(file, bone_index_size);
        int transform_level;
        read_data(file, transform_level);
        unsigned short flags;
        read_data(file, flags);
        b.flags = flags;

        if (flags & 0x0001) {
            read_index(file, bone_index_size);
        } else {
            file.seekg(12, std::ios::cur);
        }

        if (flags & 0x0100 || flags & 0x0200) {
            b.inherit_parent_index = read_index(file, bone_index_size);
            read_data(file, b.inherit_influence);
        }

        if (flags & 0x0400) file.seekg(12, std::ios::cur);
        if (flags & 0x0800) file.seekg(24, std::ios::cur);
        if (flags & 0x2000) { int key; read_data(file, key); }

        if (flags & 0x0020) {
            b.ik_target_index = read_index(file, bone_index_size);
            read_data(file, b.ik_loop_count);
            read_data(file, b.ik_limit_angle);
            int link_count;
            read_data(file, link_count);
            b.ik_links.resize(link_count);
            for (int j = 0; j < link_count; ++j) {
                b.ik_links[j].bone_index = read_index(file, bone_index_size);
                unsigned char has_limits;
                read_data(file, has_limits);
                b.ik_links[j].has_limits = (has_limits != 0);
                if (has_limits) {
                    read_data(file, b.ik_links[j].min_limit);
                    read_data(file, b.ik_links[j].max_limit);
                }
            }
        }
        data.bone_defs.push_back(b);
    }

    return data;
}

} // namespace core
