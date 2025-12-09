#ifndef _TRI_MESH_H_
#define _TRI_MESH_H_

#define GLM_FORCE_CTOR_INIT

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

typedef struct vIndex {
    unsigned int x, y, z;
    vIndex(unsigned int ix, unsigned int iy, unsigned int iz) : x(ix), y(iy), z(iz) {}
} vec3u;

struct TextureInfo
{
    std::string path;
    GLuint gl_texture_id = 0;
};

struct IKLink {
    int bone_index;
    bool has_limits;
    glm::vec3 min_limit;
    glm::vec3 max_limit;
};

struct PMXBone {
    std::string name;
    glm::vec3 position; // Initial position (Bind Pose)
    int parent_index;
    int layer;
    unsigned short flags;
    
    // Inheritance
    int inherit_parent_index = -1;
    float inherit_influence = 0.0f;

    // IK
    int ik_target_index = -1;
    int ik_loop_count = 0;
    float ik_limit_angle = 0.0f;
    std::vector<IKLink> ik_links;

    // Animation state
    glm::vec3 local_translation = glm::vec3(0.0f);
    glm::quat local_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
    // Computed transforms
    glm::mat4 global_transform = glm::mat4(1.0f);
    glm::mat4 offset_matrix = glm::mat4(1.0f); // Inverse Bind Pose Matrix
};

struct VertexBoneData {
    glm::ivec4 bone_indices = glm::ivec4(0);
    glm::vec4 bone_weights = glm::vec4(0.0f);
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

    // Transformation methods
    void setTranslation(const glm::vec3& t) { translation = t; }
    void setRotation(const glm::vec3& r) { rotation = r; }
    void setScale(const glm::vec3& s) { scale = s; }
    
    glm::vec3 getTranslation() const { return translation; }
    glm::vec3 getRotation() const { return rotation; }
    glm::vec3 getScale() const { return scale; }

    glm::mat4 getModelMatrix();

    // Getter functions
    const std::vector<glm::vec3>& getVertexPositions() const { return vertex_positions; }
    const std::vector<glm::vec3>& getVertexNormals() const { return vertex_normals; }
    const std::vector<glm::vec2>& getVertexUVs() const { return vertex_uvs; }
    const std::vector<VertexBoneData>& getVertexBoneData() const { return vertex_bone_data; }
    const std::vector<vec3u>& getFaces() const { return faces; }
    const std::vector<MaterialInfo>& getMaterials() const { return materials; }
    std::vector<TextureInfo>& getTextures() { return textures; }
    
    std::vector<PMXBone>& getBones() { return bones; }
    const std::map<std::string, int>& getBoneMapping() const { return bone_mapping; }

    void updateBoneMatrices();

private:
    std::vector<glm::vec3> vertex_positions;
    std::vector<glm::vec3> vertex_normals;
    std::vector<glm::vec2> vertex_uvs;
    std::vector<VertexBoneData> vertex_bone_data;
    std::vector<vec3u> faces;
    std::vector<MaterialInfo> materials;
    std::vector<TextureInfo> textures;
    
    std::vector<PMXBone> bones;
    std::map<std::string, int> bone_mapping;

    // Transformation state
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

#endif