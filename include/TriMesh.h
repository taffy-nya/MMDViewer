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
    bool use_internal_toon; // Added flag

    // 几何
    unsigned int num_faces;
};

class TriMesh
{
public:
    TriMesh();
    ~TriMesh();

    void read_pmx(const std::string& filename);
    void load_opengl_textures(const std::string& modelBasePath);
    void clean_data();

    // Transformation methods
    void set_translation(const glm::vec3& t) { translation = t; }
    void set_rotation(const glm::vec3& r) { rotation = r; }
    void set_scale(const glm::vec3& s) { scale = s; }
    
    glm::vec3 get_translation() const { return translation; }
    glm::vec3 get_rotation() const { return rotation; }
    glm::vec3 get_scale() const { return scale; }

    glm::mat4 get_model_matrix();

    // Getter functions
    const std::vector<glm::vec3>& get_vertex_positions() const { return vertex_positions; }
    const std::vector<glm::vec3>& get_vertex_normals() const { return vertex_normals; }
    const std::vector<glm::vec2>& get_vertex_uvs() const { return vertex_uvs; }
    const std::vector<VertexBoneData>& get_vertex_bone_data() const { return vertex_bone_data; }
    const std::vector<vec3u>& get_faces() const { return faces; }
    const std::vector<MaterialInfo>& get_materials() const { return materials; }
    std::vector<TextureInfo>& get_textures() { return textures; }
    
    std::vector<PMXBone>& get_bones() { return bones; }
    const std::map<std::string, int>& get_bone_mapping() const { return bone_mapping; }

    void update_bone_matrices();
    void reset_pose();

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