#include "model_loader.h"
#include "platform/encoding.h"
#include <fstream>
#include <format>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>

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

static void sanitize_material_flags(Material& mat) {
    static const std::vector<std::string> eye_keywords = {
        "eye", "eyewhite", "pupil", "sclera",
        "瞳", "目", "白目", "眼球", "両目", "左目", "右目"
    };
    static const std::vector<std::string> mouth_keywords = {
        "mouth", "tongue", "teeth", "tooth", "gum",
        "口", "口腔", "舌", "齿", "歯", "牙"
    };

    std::string lower = mat.name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

    auto match = [&](const std::string& kw) {
        return lower.find(kw) != std::string::npos;
    };

    for (const auto& kw : eye_keywords) {
        if (match(kw)) { mat.draw_flags &= ~0x0C; return; }
    }
    for (const auto& kw : mouth_keywords) {
        if (match(kw)) { mat.draw_flags &= ~0x0C; return; }
    }
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
    unsigned char material_index_size = g_info[4];
    unsigned char bone_index_size = g_info[5];
    unsigned char morph_index_size = g_info[6];
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
        mat.name = read_pmx_str(file, text_encoding);
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
        sanitize_material_flags(mat);
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

    // Read morphs
    int num_morphs;
    read_data(file, num_morphs);
    data.morph_defs.reserve(num_morphs);
    for (int i = 0; i < num_morphs; ++i) {
        MorphDef m;
        m.name = read_pmx_str(file, text_encoding);
        m.name_en = read_pmx_str(file, text_encoding);
        read_data(file, m.panel);
        unsigned char morph_type_raw;
        read_data(file, morph_type_raw);
        m.type = static_cast<MorphType>(morph_type_raw);

        int num_offsets;
        read_data(file, num_offsets);

        switch (m.type) {
            case MorphType::Group: {
                for (int j = 0; j < num_offsets; ++j) {
                    GroupMorphOffset off;
                    off.morph_index = read_index(file, morph_index_size);
                    read_data(file, off.weight_ratio);
                    m.group_offsets.push_back(off);
                }
                break;
            }
            case MorphType::Vertex: {
                for (int j = 0; j < num_offsets; ++j) {
                    VertexMorphOffset off;
                    off.vertex_index = read_index(file, vertex_index_size);
                    read_data(file, off.position_delta);
                    off.position_delta.z = -off.position_delta.z;
                    m.vertex_offsets.push_back(off);
                }
                break;
            }
            case MorphType::Bone: {
                for (int j = 0; j < num_offsets; ++j) {
                    BoneMorphOffset off;
                    off.bone_index = read_index(file, bone_index_size);
                    read_data(file, off.translation_delta);
                    off.translation_delta.z = -off.translation_delta.z;
                    float qx, qy, qz, qw;
                    read_data(file, qx);
                    read_data(file, qy);
                    read_data(file, qz);
                    read_data(file, qw);
                    off.rotation_quat = glm::vec4(-qx, -qy, qz, qw);
                    m.bone_offsets.push_back(off);
                }
                break;
            }
            case MorphType::UV:
            case MorphType::AdditionalUV1:
            case MorphType::AdditionalUV2:
            case MorphType::AdditionalUV3:
            case MorphType::AdditionalUV4: {
                for (int j = 0; j < num_offsets; ++j) {
                    UVMorphOffset off;
                    off.vertex_index = read_index(file, vertex_index_size);
                    read_data(file, off.uv_delta);
                    m.uv_offsets.push_back(off);
                }
                break;
            }
            case MorphType::Material: {
                for (int j = 0; j < num_offsets; ++j) {
                    MaterialMorphOffset off;
                    off.material_index = read_index(file, material_index_size);
                    read_data(file, off.calc_mode);
                    read_data(file, off.diffuse);
                    read_data(file, off.specular);
                    read_data(file, off.shininess);
                    read_data(file, off.ambient);
                    read_data(file, off.edge_color);
                    read_data(file, off.edge_size);
                    read_data(file, off.tex_coeff);
                    read_data(file, off.sphere_coeff);
                    read_data(file, off.toon_coeff);
                    m.material_offsets.push_back(off);
                }
                break;
            }
            case MorphType::Flip: {
                for (int j = 0; j < num_offsets; ++j) {
                    GroupMorphOffset off;
                    off.morph_index = read_index(file, morph_index_size);
                    read_data(file, off.weight_ratio);
                    m.group_offsets.push_back(off);
                }
                break;
            }
            case MorphType::Impulse: {
                for (int j = 0; j < num_offsets; ++j) {
                    file.seekg(morph_index_size + 12 + 12, std::ios::cur);
                }
                break;
            }
            default: {
                file.seekg(0, std::ios::cur);
                break;
            }
        }
        data.morph_defs.push_back(m);
    }

    return data;
}

} // namespace core
