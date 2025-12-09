#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "Angel.h"
#include "TriMesh.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring>

// --- Helper Functions ---
unsigned int ReadIndex(std::ifstream& file, int size) {
    unsigned int index = 0; char buffer[4]; file.read(buffer, size); if (!file) return -1;
    switch (size) {
        case 1: index = *reinterpret_cast<unsigned char*>(buffer); break;
        case 2: index = *reinterpret_cast<unsigned short*>(buffer); break;
        case 4: index = *reinterpret_cast<unsigned int*>(buffer); break;
    }
    if ((size == 1 && index == 255) || (size == 2 && index == 65535) || (size == 4 && index == 4294967295)) return -1;
    return index;
}

std::string readPmxString(std::ifstream& file, int text_encoding) {
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

void skipPmxString(std::ifstream& file) {
    int string_length; file.read(reinterpret_cast<char*>(&string_length), 4);
    if (file && string_length > 0) file.seekg(string_length, std::ios_base::cur);
}

// --- TriMesh Class Implementation ---
TriMesh::TriMesh() {}
TriMesh::~TriMesh() { cleanData(); }
void TriMesh::cleanData() { vertex_positions.clear(); vertex_normals.clear(); vertex_uvs.clear(); faces.clear(); materials.clear(); textures.clear(); }

void TriMesh::readPmx(const std::string& filename) {
    cleanData();
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) { return; }

    // --- Header ---
    char magic[4]; file.read(magic, 4);
    float version; file.read(reinterpret_cast<char*>(&version), 4);
    unsigned char global_info_size; file.read(reinterpret_cast<char*>(&global_info_size), 1);
    unsigned char g_info[8]; file.read(reinterpret_cast<char*>(g_info), global_info_size);
    unsigned char text_encoding = g_info[0], uv_size = g_info[1], vertex_index_size = g_info[2], texture_index_size = g_info[3],
                  material_index_size = g_info[4], bone_index_size = g_info[5], morph_index_size = g_info[6], rigid_body_index_size = g_info[7];
    skipPmxString(file); skipPmxString(file); skipPmxString(file); skipPmxString(file);

    // --- Vertices ---
    int num_vertices; file.read(reinterpret_cast<char*>(&num_vertices), 4);
    vertex_positions.reserve(num_vertices); vertex_normals.reserve(num_vertices); vertex_uvs.reserve(num_vertices);
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
        switch (weight_type) {
            case 0: file.seekg(bone_index_size, std::ios::cur); break;
            case 1: file.seekg(bone_index_size * 2 + 4, std::ios::cur); break;
            case 2: file.seekg(bone_index_size * 4 + 16, std::ios::cur); break;
            case 3: file.seekg(bone_index_size * 2 + 40, std::ios::cur); break;
            case 4: file.seekg(bone_index_size * 4 + 16, std::ios::cur); break;
        }
        file.seekg(4, std::ios::cur);
    }

    // --- Faces ---
    int num_face_indices; file.read(reinterpret_cast<char*>(&num_face_indices), 4);
    faces.reserve(num_face_indices / 3);
    for (int i = 0; i < num_face_indices / 3; ++i) {
        unsigned int v1 = ReadIndex(file, vertex_index_size);
        unsigned int v2 = ReadIndex(file, vertex_index_size);
        unsigned int v3 = ReadIndex(file, vertex_index_size);
        faces.emplace_back(v1, v3, v2); // Swap v2 and v3 for coordinate system
    }

    // --- Textures ---
    int num_textures; file.read(reinterpret_cast<char*>(&num_textures), 4);
    textures.reserve(num_textures);
    for (int i = 0; i < num_textures; ++i) {
        textures.push_back({readPmxString(file, text_encoding)});
    }

    // --- Materials ---
    int num_materials; file.read(reinterpret_cast<char*>(&num_materials), 4);
    materials.reserve(num_materials);
    for (int i = 0; i < num_materials; ++i) {
        MaterialInfo mat;
        skipPmxString(file); skipPmxString(file);
        file.read(reinterpret_cast<char*>(&mat.diffuse_color), 16);
        file.read(reinterpret_cast<char*>(&mat.specular_color), 12);
        file.read(reinterpret_cast<char*>(&mat.shininess), 4);
        file.read(reinterpret_cast<char*>(&mat.ambient_color), 12);
        unsigned char draw_flags; file.read(reinterpret_cast<char*>(&draw_flags), 1);
        file.read(reinterpret_cast<char*>(&mat.edge_color), 16);
        file.read(reinterpret_cast<char*>(&mat.edge_size), 4);
        mat.texture_index = ReadIndex(file, texture_index_size);
        mat.has_texture = (mat.texture_index != -1);
        file.seekg(texture_index_size + 1, std::ios::cur);
        unsigned char toon_ref_type; file.read(reinterpret_cast<char*>(&toon_ref_type), 1);
        if (toon_ref_type == 0) { mat.toon_texture_index = ReadIndex(file, texture_index_size); }
        else { unsigned char internal_index; file.read(reinterpret_cast<char*>(&internal_index), 1); mat.toon_texture_index = internal_index; }
        skipPmxString(file);
        file.read(reinterpret_cast<char*>(&mat.num_faces), 4);
        materials.push_back(mat);
    }
    file.close();
}

void TriMesh::loadOpenGLTextures(const std::string& modelBasePath) {
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

glm::mat4 TriMesh::getModelMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, translation);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}
