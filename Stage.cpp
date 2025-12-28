#include "Stage.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <string>

Stage::Stage(float size, int divisions) {
    // 生成默认场景的网格线
    std::vector<glm::vec3> line_vertices;
    float step = size * 2 / divisions;

    for (int i = 0; i <= divisions; ++i) {
        float pos = -size + i * step;
        // 平行于 X 轴的线
        line_vertices.push_back(glm::vec3(-size, 0.01f, pos)); // 稍微抬高一点
        line_vertices.push_back(glm::vec3(size, 0.01f, pos));
        // 平行于 Z 轴的线
        line_vertices.push_back(glm::vec3(pos, 0.01f, -size));
        line_vertices.push_back(glm::vec3(pos, 0.01f, size));
    }

    line_vertex_count = line_vertices.size();

    glGenVertexArrays(1, &vao_lines);
    glBindVertexArray(vao_lines);

    glGenBuffers(1, &vbo_lines);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
    glBufferData(GL_ARRAY_BUFFER, line_vertices.size() * sizeof(glm::vec3), line_vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // 生成地面平面
    float plane_data[] = {
        // 位置                  // 法线 (向上)
        -size, 0.0f, -size,     0.0f, 1.0f, 0.0f,
        -size, 0.0f,  size,     0.0f, 1.0f, 0.0f,
            size, 0.0f,  size,     0.0f, 1.0f, 0.0f,
        -size, 0.0f, -size,     0.0f, 1.0f, 0.0f,
            size, 0.0f,  size,     0.0f, 1.0f, 0.0f,
            size, 0.0f, -size,     0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &vao_plane);
    glBindVertexArray(vao_plane);

    glGenBuffers(1, &vbo_plane);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_plane);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_data), plane_data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 加载地面平面 shader (有光照)
    program = InitShader("shaders/vshader_stage.glsl", "shaders/fshader_stage.glsl");

    model_loc = glGetUniformLocation(program, "model");
    view_loc = glGetUniformLocation(program, "view");
    proj_loc = glGetUniformLocation(program, "projection");
    color_loc = glGetUniformLocation(program, "objectColor"); 
    view_pos_loc = glGetUniformLocation(program, "viewPos");
    
    ambient_color_loc = glGetUniformLocation(program, "ambientColor");
    ambient_strength_loc = glGetUniformLocation(program, "ambientStrength");
    num_lights_loc = glGetUniformLocation(program, "numLights");

    for (int i = 0; i < 16; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        light_locations[i].position = glGetUniformLocation(program, (base + ".position").c_str());
        light_locations[i].direction = glGetUniformLocation(program, (base + ".direction").c_str());
        light_locations[i].color = glGetUniformLocation(program, (base + ".color").c_str());
        light_locations[i].intensity = glGetUniformLocation(program, (base + ".intensity").c_str());
        light_locations[i].type = glGetUniformLocation(program, (base + ".type").c_str());
        light_locations[i].constant = glGetUniformLocation(program, (base + ".constant").c_str());
        light_locations[i].linear = glGetUniformLocation(program, (base + ".linear").c_str());
        light_locations[i].quadratic = glGetUniformLocation(program, (base + ".quadratic").c_str());
        light_locations[i].enabled = glGetUniformLocation(program, (base + ".enabled").c_str());
    }

    // 阴影
    shadowMap_loc = glGetUniformLocation(program, "shadowMap");
    lightSpaceMatrix_loc = glGetUniformLocation(program, "lightSpaceMatrix");

    // 阴影生成 shader
    shadow_program = InitShader("shaders/vshader_shadow_static.glsl", "shaders/fshader_shadow.glsl");
    shadow_model_loc = glGetUniformLocation(shadow_program, "model");
    shadow_lightSpaceMatrix_loc = glGetUniformLocation(shadow_program, "lightSpaceMatrix");

    // 网格线 shader (无光照)
    program_lines = InitShader("shaders/vshader_color.glsl", "shaders/fshader_color.glsl");
    model_loc_lines = glGetUniformLocation(program_lines, "model");
    view_loc_lines = glGetUniformLocation(program_lines, "view");
    proj_loc_lines = glGetUniformLocation(program_lines, "projection");
    color_loc_lines = glGetUniformLocation(program_lines, "color");
}

Stage::~Stage() {
    if (stage_painter) delete stage_painter;
    if (stage_mesh) delete stage_mesh;

    glDeleteVertexArrays(1, &vao_lines);
    glDeleteBuffers(1, &vbo_lines);
    glDeleteVertexArrays(1, &vao_plane);
    glDeleteBuffers(1, &vbo_plane);
    glDeleteProgram(program);
    glDeleteProgram(shadow_program);
    glDeleteProgram(program_lines);
}

void Stage::load_pmx(const std::string& filename) {
    use_default_grid();

    stage_mesh = new TriMesh();
    stage_mesh->read_pmx(filename);

    std::string base_path = "";
    size_t last_slash = filename.rfind('/');
    if(last_slash == std::string::npos) last_slash = filename.rfind('\\');
    if(last_slash != std::string::npos) base_path = filename.substr(0, last_slash + 1);
    stage_mesh->load_opengl_textures(base_path);

    stage_painter = new MeshPainter();
    stage_painter->add_mesh(stage_mesh);
}

void Stage::use_default_grid() {
    if (stage_painter) { delete stage_painter; stage_painter = nullptr; }
    if (stage_mesh) { delete stage_mesh; stage_mesh = nullptr; }
}

void Stage::draw_shadow(const glm::mat4& lightSpaceMatrix) {
    if (stage_painter) {
        stage_painter->draw_shadow(lightSpaceMatrix);
        return;
    }

    glUseProgram(shadow_program);
    
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(shadow_model_loc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(shadow_lightSpaceMatrix_loc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    glBindVertexArray(vao_plane);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Stage::draw(Camera* camera, const std::vector<Light>& lights, const glm::vec3& ambientColor, float ambientStrength, GLuint shadowMap, const glm::mat4& lightSpaceMatrix) {
    if (stage_painter) {
        stage_painter->draw_meshes(camera, lights, ambientColor, ambientStrength, shadowMap, lightSpaceMatrix);
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera->get_view_matrix();
    glm::mat4 projection = camera->get_projection_matrix();
    glm::vec3 viewPos = camera->position;

    // 绘制地面平面
    glUseProgram(program);
    
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(view_pos_loc, 1, glm::value_ptr(viewPos));
    
    // 阴影相关
    glUniformMatrix4fv(lightSpaceMatrix_loc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glUniform1i(shadowMap_loc, 10);
    
    glUniform4f(color_loc, 1.0f, 1.0f, 1.0f, 1.0f); // 白色平面
    
    glUniform3fv(ambient_color_loc, 1, glm::value_ptr(ambientColor));
    glUniform1f(ambient_strength_loc, ambientStrength);
    glUniform1i(num_lights_loc, (int)lights.size());

    for (int i = 0; i < lights.size() && i < 16; ++i) {
        const auto& light = lights[i];
        const auto& locs = light_locations[i];
        glUniform3fv(locs.position, 1, glm::value_ptr(light.position));
        glUniform3fv(locs.direction, 1, glm::value_ptr(light.direction));
        glUniform3fv(locs.color, 1, glm::value_ptr(light.color));
        glUniform1f(locs.intensity, light.intensity);
        glUniform1i(locs.type, light.type);
        glUniform1f(locs.constant, light.constant);
        glUniform1f(locs.linear, light.linear);
        glUniform1f(locs.quadratic, light.quadratic);
        glUniform1i(locs.enabled, light.enabled);
    }

    glBindVertexArray(vao_plane);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 绘制网格线
    glUseProgram(program_lines);

    glUniformMatrix4fv(model_loc_lines, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_loc_lines, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(proj_loc_lines, 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(vao_lines);
    glUniform3f(color_loc_lines, 0.5f, 0.5f, 0.5f); // 灰色线条
    glDrawArrays(GL_LINES, 0, line_vertex_count);

    glBindVertexArray(0);
    glUseProgram(0);
}
