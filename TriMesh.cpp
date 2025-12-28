#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include <windows.h>

#include "Angel.h"
#include "TriMesh.h"
#include <bitset>
#include <iostream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring>

unsigned int read_index(std::ifstream& file, int size) {
    unsigned int index = 0; char buffer[4]; file.read(buffer, size); if (!file) return -1;
    switch (size) {
        case 1: index = *reinterpret_cast<unsigned char*>(buffer); break;
        case 2: index = *reinterpret_cast<unsigned short*>(buffer); break;
        case 4: index = *reinterpret_cast<unsigned int*>(buffer); break;
    }
    if ((size == 1 && index == 255) || (size == 2 && index == 65535) || (size == 4 && index == 4294967295)) return -1;
    return index;
}

std::string read_pmx_string(std::ifstream& file, int text_encoding) {
    int string_length; file.read(reinterpret_cast<char*>(&string_length), 4);
    if (!file || string_length <= 0) return "";
    std::vector<char> buffer(string_length); file.read(buffer.data(), string_length);
    if (!file) return "";
    if (text_encoding == 1) { return std::string(buffer.begin(), buffer.end()); }
    else {
        const wchar_t* w_str = reinterpret_cast<const wchar_t*>(buffer.data());
        int w_len = string_length / sizeof(wchar_t);
        int utf8_size = WideCharToMultiByte(CP_UTF8, 0, w_str, w_len, NULL, 0, NULL, NULL);
        if (utf8_size == 0) return "";
        std::string utf8_str(utf8_size, 0);
        WideCharToMultiByte(CP_UTF8, 0, w_str, w_len, &utf8_str[0], utf8_size, NULL, NULL);
        return utf8_str;
    }
}

void skip_pmx_string(std::ifstream& file) {
    int string_length; file.read(reinterpret_cast<char*>(&string_length), 4);
    if (file && string_length > 0) file.seekg(string_length, std::ios_base::cur);
}

TriMesh::TriMesh() {}
TriMesh::~TriMesh() { clean_data(); }
void TriMesh::clean_data() { vertex_positions.clear(); vertex_normals.clear(); vertex_uvs.clear(); vertex_bone_data.clear(); faces.clear(); materials.clear(); textures.clear(); bones.clear(); bone_mapping.clear(); }

void TriMesh::read_pmx(const std::string& filename) {
    clean_data();
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) { return; }

    // 头部
    char magic[4]; file.read(magic, 4);
    if (strncmp(magic, "PMX ", 4) != 0) {
        std::cerr << "Not a valid PMX file." << std::endl;
        return;
    }

    float version; file.read(reinterpret_cast<char *>(&version), 4);
    unsigned char global_info_size; file.read(reinterpret_cast<char*>(&global_info_size), 1);
    unsigned char g_info[8]; file.read(reinterpret_cast<char*>(g_info), global_info_size);
    unsigned char text_encoding = g_info[0], uv_size = g_info[1], vertex_index_size = g_info[2], texture_index_size = g_info[3],
                  material_index_size = g_info[4], bone_index_size = g_info[5], morph_index_size = g_info[6], rigid_body_index_size = g_info[7];
    // 模型名称、描述之类的，跳过
    skip_pmx_string(file); skip_pmx_string(file); skip_pmx_string(file); skip_pmx_string(file);

    // 顶点
    int num_vertices; file.read(reinterpret_cast<char*>(&num_vertices), 4);
    vertex_positions.reserve(num_vertices); vertex_normals.reserve(num_vertices); vertex_uvs.reserve(num_vertices); vertex_bone_data.reserve(num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        glm::vec3 pos, normal; glm::vec2 uv;
        file.read(reinterpret_cast<char*>(&pos), 12);
        file.read(reinterpret_cast<char*>(&normal), 12);
        file.read(reinterpret_cast<char *>(&uv), 8);
        // 超级大坑：MMD 坐标系是左手系，OpenGL 是右手系，要反转 z 轴和 uv.y
        pos.z = -pos.z; normal.z = -normal.z;
        uv.y = 1.0f - uv.y;
        vertex_positions.push_back(pos);
        vertex_normals.push_back(normal);
        vertex_uvs.push_back(uv);
        if (uv_size > 0) { file.seekg(16 * uv_size, std::ios::cur); }
        
        unsigned char weight_type; file.read(reinterpret_cast<char*>(&weight_type), 1);
        VertexBoneData boneData;
        switch (weight_type) {
            case 0: { // BDEF1 (单骨骼
                int b1 = read_index(file, bone_index_size);
                boneData.bone_indices[0] = b1;
                boneData.bone_weights[0] = 1.0f;
                break;
            } case 1: { // BDEF2 (双骨骼)
                int b1 = read_index(file, bone_index_size);
                int b2 = read_index(file, bone_index_size);
                float w1; file.read(reinterpret_cast<char*>(&w1), 4);
                boneData.bone_indices[0] = b1;
                boneData.bone_indices[1] = b2;
                boneData.bone_weights[0] = w1;
                boneData.bone_weights[1] = 1.0f - w1;
                break;
            } case 2: { // BDEF4 (四骨骼)
                int b1 = read_index(file, bone_index_size);
                int b2 = read_index(file, bone_index_size);
                int b3 = read_index(file, bone_index_size);
                int b4 = read_index(file, bone_index_size);
                float w1, w2, w3, w4;
                file.read(reinterpret_cast<char*>(&w1), 4);
                file.read(reinterpret_cast<char*>(&w2), 4);
                file.read(reinterpret_cast<char*>(&w3), 4);
                file.read(reinterpret_cast<char*>(&w4), 4);
                boneData.bone_indices[0] = b1;
                boneData.bone_indices[1] = b2;
                boneData.bone_indices[2] = b3;
                boneData.bone_indices[3] = b4;
                boneData.bone_weights[0] = w1;
                boneData.bone_weights[1] = w2;
                boneData.bone_weights[2] = w3;
                boneData.bone_weights[3] = w4;
                break;
            } case 3: { // SDEF (球形混合，这里简化为 BDEF2)
                int b1 = read_index(file, bone_index_size);
                int b2 = read_index(file, bone_index_size);
                float w1; file.read(reinterpret_cast<char*>(&w1), 4);
                file.seekg(36, std::ios::cur); // 跳过 C, R0, R1
                boneData.bone_indices[0] = b1;
                boneData.bone_indices[1] = b2;
                boneData.bone_weights[0] = w1;
                boneData.bone_weights[1] = 1.0f - w1;
                break;
            } case 4: { // QDEF (四元数混合，这里简化为 BDEF4)
                int b1 = read_index(file, bone_index_size);
                int b2 = read_index(file, bone_index_size);
                int b3 = read_index(file, bone_index_size);
                int b4 = read_index(file, bone_index_size);
                float w1, w2, w3, w4;
                file.read(reinterpret_cast<char*>(&w1), 4);
                file.read(reinterpret_cast<char*>(&w2), 4);
                file.read(reinterpret_cast<char*>(&w3), 4);
                file.read(reinterpret_cast<char*>(&w4), 4);
                boneData.bone_indices[0] = b1;
                boneData.bone_indices[1] = b2;
                boneData.bone_indices[2] = b3;
                boneData.bone_indices[3] = b4;
                boneData.bone_weights[0] = w1;
                boneData.bone_weights[1] = w2;
                boneData.bone_weights[2] = w3;
                boneData.bone_weights[3] = w4;
                break;
            }
        }
        vertex_bone_data.push_back(boneData);
        file.seekg(4, std::ios::cur); // 跳过边缘数据
    }

    // 面
    int num_face_indices; file.read(reinterpret_cast<char*>(&num_face_indices), 4);
    faces.reserve(num_face_indices / 3);
    for (int i = 0; i < num_face_indices / 3; ++i) {
        unsigned int v1 = read_index(file, vertex_index_size);
        unsigned int v2 = read_index(file, vertex_index_size);
        unsigned int v3 = read_index(file, vertex_index_size);
        faces.emplace_back(v1, v3, v2); // 交换 v2 和 v3 以适应坐标系
    }

    // 纹理
    int num_textures; file.read(reinterpret_cast<char*>(&num_textures), 4);
    textures.reserve(num_textures);
    for (int i = 0; i < num_textures; ++i) {
        textures.push_back({read_pmx_string(file, text_encoding)});
    }

    // 材质
    int num_materials; file.read(reinterpret_cast<char*>(&num_materials), 4);
    materials.reserve(num_materials);
    for (int i = 0; i < num_materials; ++i) {
        MaterialInfo mat;
        skip_pmx_string(file); skip_pmx_string(file);
        file.read(reinterpret_cast<char*>(&mat.diffuse_color), 16);
        file.read(reinterpret_cast<char*>(&mat.specular_color), 12);
        file.read(reinterpret_cast<char*>(&mat.shininess), 4);
        file.read(reinterpret_cast<char*>(&mat.ambient_color), 12);
        unsigned char draw_flags; file.read(reinterpret_cast<char*>(&draw_flags), 1);
        mat.draw_flags = draw_flags;
        file.read(reinterpret_cast<char*>(&mat.edge_color), 16);
        file.read(reinterpret_cast<char*>(&mat.edge_size), 4);
        mat.texture_index = read_index(file, texture_index_size);
        mat.has_texture = (mat.texture_index != -1);
        file.seekg(texture_index_size + 1, std::ios::cur);
        unsigned char toon_ref_type; file.read(reinterpret_cast<char*>(&toon_ref_type), 1);
        if (toon_ref_type == 0) { 
            mat.toon_texture_index = read_index(file, texture_index_size); 
            mat.use_internal_toon = false;
        } else { 
            unsigned char internal_index; file.read(reinterpret_cast<char*>(&internal_index), 1); 
            mat.toon_texture_index = internal_index; 
            mat.use_internal_toon = true;
        }
        skip_pmx_string(file);
        file.read(reinterpret_cast<char*>(&mat.num_faces), 4);
        materials.push_back(mat);
    }

    // 骨骼
    int num_bones; file.read(reinterpret_cast<char*>(&num_bones), 4);
    bones.resize(num_bones);
    
    for (int i = 0; i < num_bones; ++i) {
        PMXBone& bone = bones[i];
        bone.name = read_pmx_string(file, text_encoding);
        bone_mapping[bone.name] = i;
        skip_pmx_string(file); // 跳过英文名
        file.read(reinterpret_cast<char*>(&bone.position), 12);
        bone.position.z = -bone.position.z; // 坐标系转换
        bone.parent_index = read_index(file, bone_index_size);
        int transform_level; file.read(reinterpret_cast<char*>(&transform_level), 4);
        unsigned short flags; file.read(reinterpret_cast<char*>(&flags), 2);
        bone.flags = flags;
        
        if (flags & 0x0001) { // 连接方式: 1 = 骨骼索引, 0 = 偏移量
            read_index(file, bone_index_size);
        } else {
            file.seekg(12, std::ios::cur);
        }
        
        if (flags & 0x0100 || flags & 0x0200) { // 旋转/平移 继承
             bone.inherit_parent_index = read_index(file, bone_index_size);
             file.read(reinterpret_cast<char*>(&bone.inherit_influence), 4);
        }
        
        if (flags & 0x0400) { // 固定轴
            file.seekg(12, std::ios::cur);
        }
        
        if (flags & 0x0800) { // 本地轴
            file.seekg(24, std::ios::cur);
        }
        
        if (flags & 0x2000) { // 外部父骨骼变形
            int key; file.read(reinterpret_cast<char*>(&key), 4);
        }

        if (flags & 0x0020) { // IK (反向动力学)
            bone.ik_target_index = read_index(file, bone_index_size);
            file.read(reinterpret_cast<char*>(&bone.ik_loop_count), 4);
            file.read(reinterpret_cast<char*>(&bone.ik_limit_angle), 4);
            int link_count; file.read(reinterpret_cast<char*>(&link_count), 4);
            bone.ik_links.resize(link_count);
            for(int j=0; j<link_count; ++j) {
                bone.ik_links[j].bone_index = read_index(file, bone_index_size);
                unsigned char has_limits; file.read(reinterpret_cast<char*>(&has_limits), 1);
                bone.ik_links[j].has_limits = (has_limits != 0);
                if(has_limits) {
                    file.read(reinterpret_cast<char*>(&bone.ik_links[j].min_limit), 12);
                    file.read(reinterpret_cast<char*>(&bone.ik_links[j].max_limit), 12);
                }
            }
        }
        
        // 计算偏移矩阵 (逆绑定姿势矩阵)
        // 用于将顶点从模型空间变换到骨骼空间
        bone.offset_matrix = glm::translate(glm::mat4(1.0f), -bone.position);
    }

    file.close();
}

// 更新单个骨骼的全局变换
void update_global_transform(std::vector<PMXBone>& bones, int index) {
    PMXBone& bone = bones[index];
    glm::vec3 parentPos = glm::vec3(0.0f);
    glm::mat4 parentGlobal = glm::mat4(1.0f);
    
    if (bone.parent_index != -1) {
        parentPos = bones[bone.parent_index].position;
        parentGlobal = bones[bone.parent_index].global_transform;
    }
    
    glm::vec3 relativePos = bone.position - parentPos;
    
    // 应用继承 (旋转)
    glm::quat rotation = bone.local_rotation;
    if (bone.inherit_parent_index != -1 && (bone.flags & 0x0100)) { // 旋转继承
        PMXBone& inheritParent = bones[bone.inherit_parent_index];
        glm::quat parentRot = inheritParent.local_rotation;
        if (bone.inherit_influence != 1.0f) {
            parentRot = glm::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), parentRot, bone.inherit_influence);
        }
        rotation = parentRot * rotation;
    }

    // 应用继承 (平移)
    glm::vec3 translation = bone.local_translation;
    if (bone.inherit_parent_index != -1 && (bone.flags & 0x0200)) { // 平移继承
        PMXBone& inheritParent = bones[bone.inherit_parent_index];
        glm::vec3 parentTrans = inheritParent.local_translation;
        translation += parentTrans * bone.inherit_influence;
    }

    glm::mat4 local = glm::mat4(1.0f);
    local = glm::translate(local, relativePos + translation);
    local = local * glm::mat4_cast(rotation);
    
    bone.global_transform = parentGlobal * local;
}

// 基于 CCD 的 IK 求解
void solve_ik(std::vector<PMXBone>& bones, int ikBoneIndex) {
    PMXBone& ikBone = bones[ikBoneIndex];
    if (ikBone.ik_target_index == -1) return;

    PMXBone& targetBone = bones[ikBone.ik_target_index];

    // IK 循环
    for (int iter = 0; iter < ikBone.ik_loop_count; ++iter) {
        bool converged = true;
        
        for (int i = 0; i < ikBone.ik_links.size(); ++i) {
            IKLink& link = ikBone.ik_links[i];
            PMXBone& linkBone = bones[link.bone_index];
            
            // 当前末端执行器位置
            glm::vec3 effectorPos = glm::vec3(targetBone.global_transform[3]);
            glm::vec3 targetPos = glm::vec3(ikBone.global_transform[3]); // IK 骨骼是目标位置
            
            glm::vec3 linkPos = glm::vec3(linkBone.global_transform[3]);
            
            glm::vec3 toEffector = glm::normalize(effectorPos - linkPos);
            glm::vec3 toTarget = glm::normalize(targetPos - linkPos);
            
            // 计算将末端执行器带到目标的旋转
            float dot = glm::dot(toEffector, toTarget);
            if (glm::abs(dot) > 0.9999f) continue; // 已经对齐
            
            float angle = glm::acos(glm::clamp(dot, -1.0f, 1.0f));
            if (glm::abs(angle) < 0.0001f) continue;

            if (ikBone.ik_limit_angle > 0.0f) {
                angle = glm::min(angle, ikBone.ik_limit_angle);
            }
            
            glm::vec3 axis = glm::cross(toEffector, toTarget);
            if (glm::length(axis) < 0.0001f) continue;
            axis = glm::normalize(axis);
            
            // 将轴转换为 linkBone 的局部空间
            glm::mat4 parentGlobal = glm::mat4(1.0f);
            if (linkBone.parent_index != -1) {
                parentGlobal = bones[linkBone.parent_index].global_transform;
            }
            glm::mat3 invParentRot = glm::transpose(glm::mat3(parentGlobal));
            glm::vec3 localAxis = invParentRot * axis;
            
            glm::quat deltaRot = glm::angleAxis(angle, glm::normalize(localAxis));
            linkBone.local_rotation = deltaRot * linkBone.local_rotation;
            linkBone.local_rotation = glm::normalize(linkBone.local_rotation);
            
            // 应用限制
            if (link.has_limits) {
                // 简单的膝盖限制 (min/max y 和 z 的限制接近 0)
                if (glm::abs(link.min_limit.y) < 0.001f && glm::abs(link.max_limit.y) < 0.001f &&
                    glm::abs(link.min_limit.z) < 0.001f && glm::abs(link.max_limit.z) < 0.001f) {
                    // 提取当前绕 X 轴的旋转角度
                    float angle = 2.0f * std::atan2(linkBone.local_rotation.x, linkBone.local_rotation.w);
                    
                    // 限制角度
                    angle = glm::clamp(angle, link.min_limit.x, link.max_limit.x);

                    // 计算旋转
                    linkBone.local_rotation = glm::angleAxis(angle, glm::vec3(1.0f, 0.0f, 0.0f));
                }
            }
            
            // 更新链
            update_global_transform(bones, link.bone_index);
            for (int k = i - 1; k >= 0; --k) {
                 update_global_transform(bones, ikBone.ik_links[k].bone_index);
            }
            update_global_transform(bones, ikBone.ik_target_index);
            
            converged = false;
        }
        
        if (converged) break;
    }
}

void TriMesh::update_bone_matrices() {
    // 重置 FK
    // 根据父子关系计算所有骨骼的全局变换
    for (int i = 0; i < bones.size(); ++i) {
        update_global_transform(bones, i);
    }
    
    // IK
    for (int i = 0; i < bones.size(); ++i) {
        if (bones[i].ik_target_index != -1) {
            solve_ik(bones, i);
        }
    }
}

void TriMesh::reset_pose() {
    for (auto& bone : bones) {
        bone.local_translation = glm::vec3(0.0f);
        bone.local_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
}


static std::wstring to_wstring(const std::string& str, UINT codepage) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void TriMesh::load_opengl_textures(const std::string& modelBasePath) {
    stbi_set_flip_vertically_on_load(true);
    for (auto& texInfo : textures) {
        if (texInfo.path.empty()) continue;

        int width, height, nrChannels;
        unsigned char* data = nullptr;
        std::string reason = "Unknown error";

        // 避免编码问题，用 UTF-8 读取路径
        // modelBasePath 可能是 ANSI
        std::wstring wBasePath = to_wstring(modelBasePath, CP_ACP);
        // texInfo.path 是 UTF-8
        std::wstring wTexPath = to_wstring(texInfo.path, CP_UTF8);
        
        std::wstring wFullPath = wBasePath + wTexPath;
        for (wchar_t& c : wFullPath) if (c == L'\\') c = L'/';

        // 用 stb_image 读取纹理
        FILE* f = _wfopen(wFullPath.c_str(), L"rb");
        if (f) {
            data = stbi_load_from_file(f, &width, &height, &nrChannels, 0);
            if (!data) reason = stbi_failure_reason();
            fclose(f);
        } else {
            reason = "Unable to open file";
        }

        if (data && width > 0 && height > 0) {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED; else if (nrChannels == 4) format = GL_RGBA;
            glGenTextures(1, &texInfo.gl_texture_id);
            glBindTexture(GL_TEXTURE_2D, texInfo.gl_texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } else {
            std::string fullPath = modelBasePath + texInfo.path;
            std::cerr << "Failed to load texture: " << fullPath << " (Reason: " << reason << ")" << std::endl;
            texInfo.gl_texture_id = 0;
        }
        if (data) stbi_image_free(data);
    }
}

glm::mat4 TriMesh::get_model_matrix() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, translation);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}
