#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#ifdef _WIN32
#include <windows.h>
#endif

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

// --- Helper Functions ---
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

// --- TriMesh Class Implementation ---
TriMesh::TriMesh() {}
TriMesh::~TriMesh() { clean_data(); }
void TriMesh::clean_data() { vertex_positions.clear(); vertex_normals.clear(); vertex_uvs.clear(); vertex_bone_data.clear(); faces.clear(); materials.clear(); textures.clear(); bones.clear(); bone_mapping.clear(); }

void TriMesh::read_pmx(const std::string& filename) {
    clean_data();
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) { return; }

    // --- Header ---
    char magic[4]; file.read(magic, 4);
    float version; file.read(reinterpret_cast<char*>(&version), 4);
    unsigned char global_info_size; file.read(reinterpret_cast<char*>(&global_info_size), 1);
    unsigned char g_info[8]; file.read(reinterpret_cast<char*>(g_info), global_info_size);
    unsigned char text_encoding = g_info[0], uv_size = g_info[1], vertex_index_size = g_info[2], texture_index_size = g_info[3],
                  material_index_size = g_info[4], bone_index_size = g_info[5], morph_index_size = g_info[6], rigid_body_index_size = g_info[7];
    skip_pmx_string(file); skip_pmx_string(file); skip_pmx_string(file); skip_pmx_string(file);

    // --- Vertices ---
    int num_vertices; file.read(reinterpret_cast<char*>(&num_vertices), 4);
    vertex_positions.reserve(num_vertices); vertex_normals.reserve(num_vertices); vertex_uvs.reserve(num_vertices); vertex_bone_data.reserve(num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        glm::vec3 pos, normal; glm::vec2 uv;
        file.read(reinterpret_cast<char*>(&pos), 12);
        file.read(reinterpret_cast<char*>(&normal), 12);
        file.read(reinterpret_cast<char*>(&uv), 8);
        pos.z = -pos.z; normal.z = -normal.z; // Coordinate system conversion
        uv.y = 1.0f - uv.y; // UV coordinate conversion
        vertex_positions.push_back(pos);
        vertex_normals.push_back(normal);
        vertex_uvs.push_back(uv);
        if (uv_size > 0) { file.seekg(16 * uv_size, std::ios::cur); }
        
        unsigned char weight_type; file.read(reinterpret_cast<char*>(&weight_type), 1);
        VertexBoneData boneData;
        switch (weight_type) {
            case 0: // BDEF1
            {
                int b1 = read_index(file, bone_index_size);
                boneData.bone_indices[0] = b1;
                boneData.bone_weights[0] = 1.0f;
                break;
            }
            case 1: // BDEF2
            {
                int b1 = read_index(file, bone_index_size);
                int b2 = read_index(file, bone_index_size);
                float w1; file.read(reinterpret_cast<char*>(&w1), 4);
                boneData.bone_indices[0] = b1;
                boneData.bone_indices[1] = b2;
                boneData.bone_weights[0] = w1;
                boneData.bone_weights[1] = 1.0f - w1;
                break;
            }
            case 2: // BDEF4
            {
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
            case 3: // SDEF
            {
                int b1 = read_index(file, bone_index_size);
                int b2 = read_index(file, bone_index_size);
                float w1; file.read(reinterpret_cast<char*>(&w1), 4);
                file.seekg(36, std::ios::cur); // Skip C, R0, R1
                boneData.bone_indices[0] = b1;
                boneData.bone_indices[1] = b2;
                boneData.bone_weights[0] = w1;
                boneData.bone_weights[1] = 1.0f - w1;
                break;
            }
            case 4: // QDEF
            {
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
        file.seekg(4, std::ios::cur); // Edge scale
    }

    // --- Faces ---
    int num_face_indices; file.read(reinterpret_cast<char*>(&num_face_indices), 4);
    faces.reserve(num_face_indices / 3);
    for (int i = 0; i < num_face_indices / 3; ++i) {
        unsigned int v1 = read_index(file, vertex_index_size);
        unsigned int v2 = read_index(file, vertex_index_size);
        unsigned int v3 = read_index(file, vertex_index_size);
        faces.emplace_back(v1, v3, v2); // Swap v2 and v3 for coordinate system
    }

    // --- Textures ---
    int num_textures; file.read(reinterpret_cast<char*>(&num_textures), 4);
    textures.reserve(num_textures);
    for (int i = 0; i < num_textures; ++i) {
        textures.push_back({read_pmx_string(file, text_encoding)});
    }

    // --- Materials ---
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
        file.read(reinterpret_cast<char*>(&mat.edge_color), 16);
        file.read(reinterpret_cast<char*>(&mat.edge_size), 4);
        mat.texture_index = read_index(file, texture_index_size);
        mat.has_texture = (mat.texture_index != -1);
        file.seekg(texture_index_size + 1, std::ios::cur);
        unsigned char toon_ref_type; file.read(reinterpret_cast<char*>(&toon_ref_type), 1);
        if (toon_ref_type == 0) { mat.toon_texture_index = read_index(file, texture_index_size); }
        else { unsigned char internal_index; file.read(reinterpret_cast<char*>(&internal_index), 1); mat.toon_texture_index = internal_index; }
        skip_pmx_string(file);
        file.read(reinterpret_cast<char*>(&mat.num_faces), 4);
        materials.push_back(mat);
    }

    // --- Bones ---
    int num_bones; file.read(reinterpret_cast<char*>(&num_bones), 4);
    bones.resize(num_bones);
    
    for(int i=0; i<num_bones; ++i) {
        PMXBone& bone = bones[i];
        bone.name = read_pmx_string(file, text_encoding);
        bone_mapping[bone.name] = i;
        skip_pmx_string(file); // English name
        file.read(reinterpret_cast<char*>(&bone.position), 12);
        bone.position.z = -bone.position.z; // Coordinate fix
        bone.parent_index = read_index(file, bone_index_size);
        int transform_level; file.read(reinterpret_cast<char*>(&transform_level), 4);
        unsigned short flags; file.read(reinterpret_cast<char*>(&flags), 2);
        bone.flags = flags;
        
        if (flags & 0x0001) { // Connection: 1 = Bone Index, 0 = Offset
            read_index(file, bone_index_size);
        } else {
            file.seekg(12, std::ios::cur);
        }
        
        if (flags & 0x0100 || flags & 0x0200) { // Rotation/Translation Append
             bone.inherit_parent_index = read_index(file, bone_index_size);
             file.read(reinterpret_cast<char*>(&bone.inherit_influence), 4);
        }
        
        if (flags & 0x0400) { // Fixed Axis
            file.seekg(12, std::ios::cur);
        }
        
        if (flags & 0x0800) { // Local Axis
            file.seekg(24, std::ios::cur);
        }
        
        if (flags & 0x2000) { // External Parent Deform
            int key; file.read(reinterpret_cast<char*>(&key), 4);
        }

        if (flags & 0x0020) { // IK
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
        
        // Calculate Offset Matrix (Inverse Bind Pose)
        bone.offset_matrix = glm::translate(glm::mat4(1.0f), -bone.position);
    }

    file.close();
}

// Helper to update a single bone's global transform
void update_global_transform(std::vector<PMXBone>& bones, int index) {
    PMXBone& bone = bones[index];
    glm::vec3 parentPos = glm::vec3(0.0f);
    glm::mat4 parentGlobal = glm::mat4(1.0f);
    
    if (bone.parent_index != -1) {
        parentPos = bones[bone.parent_index].position;
        parentGlobal = bones[bone.parent_index].global_transform;
    }
    
    glm::vec3 relativePos = bone.position - parentPos;
    
    // Apply Inheritance (Rotation)
    glm::quat rotation = bone.local_rotation;
    if (bone.inherit_parent_index != -1 && (bone.flags & 0x0100)) { // Rotation Inherit
        PMXBone& inheritParent = bones[bone.inherit_parent_index];
        glm::quat parentRot = inheritParent.local_rotation;
        if (bone.inherit_influence != 1.0f) {
            parentRot = glm::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), parentRot, bone.inherit_influence);
        }
        rotation = parentRot * rotation;
    }

    // Apply Inheritance (Translation)
    glm::vec3 translation = bone.local_translation;
    if (bone.inherit_parent_index != -1 && (bone.flags & 0x0200)) { // Translation Inherit
        PMXBone& inheritParent = bones[bone.inherit_parent_index];
        glm::vec3 parentTrans = inheritParent.local_translation;
        translation += parentTrans * bone.inherit_influence;
    }

    glm::mat4 local = glm::mat4(1.0f);
    local = glm::translate(local, relativePos + translation);
    local = local * glm::mat4_cast(rotation);
    
    bone.global_transform = parentGlobal * local;
}

void solve_ik(std::vector<PMXBone>& bones, int ikBoneIndex) {
    PMXBone& ikBone = bones[ikBoneIndex];
    if (ikBone.ik_target_index == -1) return;

    PMXBone& targetBone = bones[ikBone.ik_target_index];

    // IK Loop
    for (int iter = 0; iter < ikBone.ik_loop_count; ++iter) {
        bool converged = true;
        
        for (int i = 0; i < ikBone.ik_links.size(); ++i) {
            IKLink& link = ikBone.ik_links[i];
            PMXBone& linkBone = bones[link.bone_index];
            
            // Current Effector Position (Target Bone's current global position)
            // We need to re-calculate global transforms for the chain to get accurate positions
            // But for performance, we assume they are updated or we update them as we go.
            // Since we modify linkBone, we must update the chain from linkBone downwards.
            
            // For simplicity in this iteration, let's just use current global_transform[3]
            // But we really should update the chain.
            // Let's assume UpdateGlobalTransform is called for the whole chain before IK.
            
            glm::vec3 effectorPos = glm::vec3(targetBone.global_transform[3]);
            glm::vec3 targetPos = glm::vec3(ikBone.global_transform[3]); // The IK bone is the goal position
            
            glm::vec3 linkPos = glm::vec3(linkBone.global_transform[3]);
            
            glm::vec3 toEffector = glm::normalize(effectorPos - linkPos);
            glm::vec3 toTarget = glm::normalize(targetPos - linkPos);
            
            // Calculate rotation to bring effector to target
            float dot = glm::dot(toEffector, toTarget);
            if (glm::abs(dot) > 0.9999f) continue; // Already aligned
            
            float angle = glm::acos(glm::clamp(dot, -1.0f, 1.0f));
            if (glm::abs(angle) < 0.0001f) continue;

            if (ikBone.ik_limit_angle > 0.0f) {
                angle = glm::min(angle, ikBone.ik_limit_angle);
            }
            
            glm::vec3 axis = glm::cross(toEffector, toTarget);
            if (glm::length(axis) < 0.0001f) continue;
            axis = glm::normalize(axis);
            
            // Convert axis to local space of linkBone
            // Parent Global Rotation * Local Rotation = Global Rotation
            // So Local Axis = inverse(Parent Global Rotation) * Global Axis
            glm::mat4 parentGlobal = glm::mat4(1.0f);
            if (linkBone.parent_index != -1) {
                parentGlobal = bones[linkBone.parent_index].global_transform;
            }
            glm::mat3 invParentRot = glm::transpose(glm::mat3(parentGlobal));
            glm::vec3 localAxis = invParentRot * axis;
            
            glm::quat deltaRot = glm::angleAxis(angle, glm::normalize(localAxis));
            linkBone.local_rotation = deltaRot * linkBone.local_rotation;
            linkBone.local_rotation = glm::normalize(linkBone.local_rotation);
            
            // Apply Limits
            if (link.has_limits) {
                // Simple Knee Limit (X-axis only)
                // Check if it's a knee-like limit (min/max y and z are near 0)
                if (glm::abs(link.min_limit.y) < 0.001f && glm::abs(link.max_limit.y) < 0.001f &&
                    glm::abs(link.min_limit.z) < 0.001f && glm::abs(link.max_limit.z) < 0.001f) {
                    
                    // Decompose to Euler angles (XYZ)
                    // Since we want to restrict to X-axis, we can try to extract the X rotation.
                    // A robust way is to convert the current local_rotation to Euler, clamp X, and reset Y/Z to 0.
                    // However, standard glm::eulerAngles is RGB (Pitch/Yaw/Roll) which depends on order.
                    // MMD uses ZXY order for Euler? Or XYZ?
                    // Actually, for a 1-DOF joint, we can just project the rotation axis.
                    
                    // Let's assume the bone's local coordinate system is set up such that X is the hinge.
                    // We want to keep only the rotation around X.
                    // q = [sin(theta/2) * axis, cos(theta/2)]
                    // We want axis = (1,0,0).
                    // So q should be (sin(theta/2), 0, 0, cos(theta/2)).
                    
                    // Extract X component of rotation
                    // This is an approximation.
                    float angle = 2.0f * std::atan2(linkBone.local_rotation.x, linkBone.local_rotation.w);
                    
                    // Clamp angle
                    // min_limit and max_limit are in radians
                    angle = glm::clamp(angle, link.min_limit.x, link.max_limit.x);
                    
                    // Reconstruct quaternion
                    linkBone.local_rotation = glm::angleAxis(angle, glm::vec3(1.0f, 0.0f, 0.0f));
                }
            }
            
            // Update chain
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
    // 1. Reset / FK Pass
    for (int i = 0; i < bones.size(); ++i) {
        update_global_transform(bones, i);
    }
    
    // 2. IK Pass
    for (int i = 0; i < bones.size(); ++i) {
        if (bones[i].ik_target_index != -1) {
            solve_ik(bones, i);
        }
    }
}


void TriMesh::load_opengl_textures(const std::string& modelBasePath) {
    stbi_set_flip_vertically_on_load(true);
    for (auto& texInfo : textures) {
        if (texInfo.path.empty()) continue;
        std::string fullPath = modelBasePath + texInfo.path;
        for (char& c : fullPath) if (c == '\\') c = '/';
        int width, height, nrChannels;
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);
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
            std::cerr << "Failed to load texture: " << fullPath << " (Reason: " << stbi_failure_reason() << ")" << std::endl;
            texInfo.gl_texture_id = 0;
        }
        stbi_image_free(data);
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
