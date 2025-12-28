#ifndef _MESH_PAINTER_H_
#define _MESH_PAINTER_H_

#include "TriMesh.h"
#include "Camera.h"
#include "Light.h"
#include <vector>
#include <string>

// 着色器配置
struct ShaderConfig {
    std::string vshader;      // 主渲染
    std::string fshader;
    std::string vshader_edge; // 描边
    std::string fshader_edge;

    static ShaderConfig default_mmd() {
        return {
            "shaders/vshader.glsl",     
            "shaders/fshader_with_light.glsl",
            "shaders/vshader_edge.glsl",
            "shaders/fshader_edge.glsl"
        };
    }
};


struct OpenGLObject {
    GLuint vao = 0, vbo = 0, ebo = 0, program = 0;
    
    // Uniform
    GLuint model_location, view_location, projection_location;
    GLuint object_color_location, view_pos_location;
    GLuint has_texture_location, shininess_location, specular_color_location;
    GLuint edge_size_location, edge_color_location;            // 描边 shader 的参数
    GLuint texture_sampler_location, toon_sampler_location;    // 纹理
    GLuint bone_matrices_location;                             // 骨骼矩阵数组
    
    // 阴影贴图
    GLuint shadowMap_location;        // 深度纹理采样器
    GLuint lightSpaceMatrix_location; // 光源空间变换矩阵

    // 阴影生成
    GLuint shadow_program = 0;
    GLuint shadow_model_location, shadow_lightSpaceMatrix_location, shadow_boneMatrices_location;

    // 光照
    GLuint ambient_color_location, ambient_strength_location;
    GLuint num_lights_location;
    GLuint brightness_location;
    struct LightLocs {
        GLuint position, direction, color, intensity, type, constant, linear, quadratic, enabled;
    } light_locations[16]; // 最多 16 个光源

    // 用于存储大量骨骼矩阵
    GLuint bone_tbo = 0;
    GLuint bone_texture = 0;
};

class MeshPainter
{
public:
    MeshPainter();
    ~MeshPainter();

    // 绘制一个 mesh
    void add_mesh(TriMesh* mesh, const ShaderConfig& config = ShaderConfig::default_mmd());
    
    // 绘制所有 mesh
    void draw_meshes(Camera* camera, const std::vector<Light>& lights, const glm::vec3& ambientColor, float ambientStrength, GLuint shadowMap, const glm::mat4& lightSpaceMatrix, float brightness = 1.0f);
    
    // 绘制阴影贴图
    void draw_shadow(const glm::mat4& lightSpaceMatrix);
    
    // 绘制光源图标
    void draw_light_gizmos(Camera* camera, const std::vector<Light>& lights);
    
    // 绘制骨骼辅助线
    void draw_skeleton(Camera* camera, int selected_bone_index = -1);
    
    void clean_up();

private:
    struct MeshEntry {
        TriMesh* mesh;
        OpenGLObject main_object;
        OpenGLObject edge_object;
    };

    std::vector<MeshEntry> meshes;

    void bind_object_and_data(TriMesh* mesh, OpenGLObject& object, const std::string& vshader, const std::string& fshader, bool isEdge);

    GLuint gizmo_program = 0;
    GLuint gizmo_vao = 0, gizmo_vbo = 0;
    GLuint gizmo_line_vao = 0, gizmo_line_vbo = 0;
    GLuint gizmo_model_loc, gizmo_view_loc, gizmo_proj_loc, gizmo_color_loc;
    
    GLuint default_toon_texture = 0;
    void init_gizmo_resources();
};

#endif
