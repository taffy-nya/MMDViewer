#ifndef _TRI_MESH_H_
#define _TRI_MESH_H_

#define GLM_FORCE_CTOR_INIT

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


typedef struct vIndex {
    unsigned int x, y, z;
    vIndex(unsigned int ix, unsigned int iy, unsigned int iz) : x(ix), y(iy), z(iz) {}
} vec3u;

struct TextureInfo
{
    std::string path;
    GLuint gl_texture_id = 0;
};

// 材质信息结构
struct MaterialInfo {
    // 基础光照
    glm::vec4 diffuse_color;
    glm::vec3 specular_color;
    float shininess;
    glm::vec3 ambient_color;

    // 轮廓线
    glm::vec4 edge_color;
    float edge_size;

    // 纹理
    bool has_texture;
    int texture_index;
    int toon_texture_index;

    // 几何
    unsigned int num_faces;
};

class TriMesh
{
public:
    TriMesh();
    ~TriMesh();

    void readPmx(const std::string& filename);
    void loadOpenGLTextures(const std::string& modelBasePath);
    void cleanData();

    // Getter functions
    const std::vector<glm::vec3>& getVertexPositions() const { return vertex_positions; }
    const std::vector<glm::vec3>& getVertexNormals() const { return vertex_normals; }
    const std::vector<glm::vec2>& getVertexUVs() const { return vertex_uvs; }
    const std::vector<vec3u>& getFaces() const { return faces; }
    const std::vector<MaterialInfo>& getMaterials() const { return materials; }
    std::vector<TextureInfo>& getTextures() { return textures; }

private:
    std::vector<glm::vec3> vertex_positions;
    std::vector<glm::vec3> vertex_normals;
    std::vector<glm::vec2> vertex_uvs;
    std::vector<vec3u> faces;
    std::vector<MaterialInfo> materials;
    std::vector<TextureInfo> textures;
};

#endif