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

// IK (反向动力学) 链接结构
struct IKLink {
    int bone_index;
    bool has_limits;     // 角度限制
    glm::vec3 min_limit; // 最小角度限制
    glm::vec3 max_limit; // 最大角度限制
};

// 骨骼结构
struct PMXBone {
    std::string name;
    glm::vec3 position; // 初始位置
    int parent_index;   // 父骨骼索引
    int layer;
    unsigned short flags;
    
    // 继承
    int inherit_parent_index = -1;
    float inherit_influence = 0.0f;

    // IK
    int ik_target_index = -1; // IK 目标骨骼索引
    int ik_loop_count = 0;    // 迭代次数
    float ik_limit_angle = 0.0f; // 每次迭代的限制角度
    std::vector<IKLink> ik_links; // IK 链

    // 动画状态
    glm::vec3 local_translation = glm::vec3(0.0f);
    glm::quat local_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
    // 计算出的变换矩阵
    glm::mat4 global_transform = glm::mat4(1.0f); // 全局变换矩阵
    glm::mat4 offset_matrix = glm::mat4(1.0f);    // 逆绑定姿势矩阵
};

// 顶点骨骼数据 (用于蒙皮)
struct VertexBoneData {
    glm::ivec4 bone_indices = glm::ivec4(0); // 影响该顶点的骨骼索引 (最多4个)
    glm::vec4 bone_weights = glm::vec4(0.0f); // 对应的权重
};

// 材质信息
struct MaterialInfo {
    // 光照
    glm::vec4 diffuse_color;  // 漫反射
    glm::vec3 specular_color; // 镜面反射
    float shininess;          // 高光
    glm::vec3 ambient_color;  // 环境光

    // 描边
    glm::vec4 edge_color;
    float edge_size;

    // 纹理
    bool has_texture;
    int texture_index;
    int toon_texture_index;
    bool use_internal_toon; // 是否使用内置 Toon 纹理 (未实现)

    unsigned int num_faces;   // 该材质包含的面数
    unsigned char draw_flags; // 绘制标志 (0x01 = 双面绘制)
};

class TriMesh
{
public:
    TriMesh();
    ~TriMesh();

    // 读取 PMX 模型文件
    void read_pmx(const std::string& filename);
    // 加载纹理到 OpenGL
    void load_opengl_textures(const std::string& modelBasePath);
    void clean_data();

    // 设置变换
    void set_translation(const glm::vec3& t) { translation = t; }
    void set_rotation(const glm::vec3& r) { rotation = r; }
    void set_scale(const glm::vec3& s) { scale = s; }
    
    glm::vec3 get_translation() const { return translation; }
    glm::vec3 get_rotation() const { return rotation; }
    glm::vec3 get_scale() const { return scale; }

    glm::mat4 get_model_matrix();

    // 一堆 getter
    const std::vector<glm::vec3>& get_vertex_positions() const { return vertex_positions; }
    const std::vector<glm::vec3>& get_vertex_normals() const { return vertex_normals; }
    const std::vector<glm::vec2>& get_vertex_uvs() const { return vertex_uvs; }
    const std::vector<VertexBoneData>& get_vertex_bone_data() const { return vertex_bone_data; }
    const std::vector<vec3u>& get_faces() const { return faces; }
    const std::vector<MaterialInfo>& get_materials() const { return materials; }
    std::vector<TextureInfo>& get_textures() { return textures; }
    
    std::vector<PMXBone>& get_bones() { return bones; }
    const std::map<std::string, int>& get_bone_mapping() const { return bone_mapping; }

    // 更新骨骼矩阵 (FK 和 IK)
    void update_bone_matrices();
    // 重置姿态
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

    // 变换状态
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

#endif