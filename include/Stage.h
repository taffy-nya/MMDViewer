#ifndef _STAGE_H_
#define _STAGE_H_

#include "Angel.h"
#include "Camera.h"
#include "Light.h"
#include <vector>

class Stage {
public:
    Stage(float size = 50.0f, int divisions = 20);
    ~Stage();
    void draw(Camera* camera, const std::vector<Light>& lights, const glm::vec3& ambientColor, float ambientStrength, GLuint shadowMap, const glm::mat4& lightSpaceMatrix);
    void draw_shadow(const glm::mat4& lightSpaceMatrix);

private:
    GLuint vao_lines, vbo_lines;
    GLuint vao_plane, vbo_plane;
    GLuint program; // For Plane (Lit)
    GLuint program_lines; // For Grid (Unlit)
    
    // Uniforms for Plane
    GLuint model_loc, view_loc, proj_loc, color_loc;
    GLuint view_pos_loc;
    GLuint ambient_color_loc, ambient_strength_loc, num_lights_loc;
    
    // Shadow Mapping
    GLuint shadowMap_loc;
    GLuint lightSpaceMatrix_loc;
    
    // Shadow Pass Program
    GLuint shadow_program;
    GLuint shadow_model_loc, shadow_lightSpaceMatrix_loc;
    
    // Uniforms for Lines
    GLuint model_loc_lines, view_loc_lines, proj_loc_lines, color_loc_lines;

    struct LightLocs {
        GLuint position, direction, color, intensity, type, constant, linear, quadratic, enabled;
    } light_locations[16];

    int line_vertex_count;
};

#endif
