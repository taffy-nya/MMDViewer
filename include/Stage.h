#ifndef _STAGE_H_
#define _STAGE_H_

#include "Angel.h"
#include "Camera.h"
#include "Light.h"
#include "TriMesh.h"
#include "MeshPainter.h"
#include <vector>
#include <string>

class Stage {
public:
    Stage(float size = 50.0f, int divisions = 20);
    ~Stage();

    // 绘制场景
    void draw(Camera* camera, const std::vector<Light>& lights, const glm::vec3& ambientColor, float ambientStrength, GLuint shadowMap, const glm::mat4& lightSpaceMatrix);
    
    // 绘制场景阴影
    void draw_shadow(const glm::mat4& lightSpaceMatrix);

    // 加载 PMX 模型
    void load_pmx(const std::string& filename);
    
    // 使用默认的网格地面
    void use_default_grid();

private:
    // 自定义 PMX 场景
    TriMesh* stage_mesh = nullptr;
    MeshPainter* stage_painter = nullptr;

    // 默认的网格地面
    GLuint vao_lines, vbo_lines; // 网格线
    GLuint vao_plane, vbo_plane; // 地面平面
    GLuint program;              // 地面平面 shader (有光照)
    GLuint program_lines;        // 网格线 shader (无光照)
    
    // 地面平面
    GLuint model_loc, view_loc, proj_loc, color_loc;
    GLuint view_pos_loc;
    GLuint ambient_color_loc, ambient_strength_loc, num_lights_loc;
    
    // 阴影贴图
    GLuint shadowMap_loc;
    GLuint lightSpaceMatrix_loc;
    
    // 阴影生成
    GLuint shadow_program;
    GLuint shadow_model_loc, shadow_lightSpaceMatrix_loc;
    
    // 网格线
    GLuint model_loc_lines, view_loc_lines, proj_loc_lines, color_loc_lines;

    struct LightLocs {
        GLuint position, direction, color, intensity, type, constant, linear, quadratic, enabled;
    } light_locations[16];

    int line_vertex_count;
};

#endif
