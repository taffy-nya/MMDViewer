#include "Stage.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

Stage::Stage(float size, int divisions) {
    // 1. Generate Grid Lines
    std::vector<glm::vec3> line_vertices;
    float step = size * 2 / divisions;

    for (int i = 0; i <= divisions; ++i) {
        float pos = -size + i * step;
        // Line parallel to X-axis
        line_vertices.push_back(glm::vec3(-size, 0.01f, pos)); // Slightly raised to avoid z-fighting
        line_vertices.push_back(glm::vec3(size, 0.01f, pos));
        // Line parallel to Z-axis
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

    // 2. Generate Plane
    std::vector<glm::vec3> plane_vertices = {
        glm::vec3(-size, 0.0f, -size),
        glm::vec3(-size, 0.0f, size),
        glm::vec3(size, 0.0f, size),
        glm::vec3(-size, 0.0f, -size),
        glm::vec3(size, 0.0f, size),
        glm::vec3(size, 0.0f, -size)
    };

    glGenVertexArrays(1, &vao_plane);
    glBindVertexArray(vao_plane);

    glGenBuffers(1, &vbo_plane);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_plane);
    glBufferData(GL_ARRAY_BUFFER, plane_vertices.size() * sizeof(glm::vec3), plane_vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // 3. Load Shader
    program = InitShader("shaders/vshader_stage.glsl", "shaders/fshader_stage.glsl");

    model_loc = glGetUniformLocation(program, "model");
    view_loc = glGetUniformLocation(program, "view");
    proj_loc = glGetUniformLocation(program, "projection");
    color_loc = glGetUniformLocation(program, "color");
}

Stage::~Stage() {
    glDeleteVertexArrays(1, &vao_lines);
    glDeleteBuffers(1, &vbo_lines);
    glDeleteVertexArrays(1, &vao_plane);
    glDeleteBuffers(1, &vbo_plane);
    glDeleteProgram(program);
}

void Stage::draw(Camera* camera) {
    glUseProgram(program);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera->get_view_matrix();
    glm::mat4 projection = camera->get_projection_matrix();

    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Draw Plane (White)
    glBindVertexArray(vao_plane);
    glUniform4f(color_loc, 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw Lines (Gray)
    glBindVertexArray(vao_lines);
    glUniform4f(color_loc, 0.5f, 0.5f, 0.5f, 1.0f);
    glDrawArrays(GL_LINES, 0, line_vertex_count);

    glBindVertexArray(0);
    glUseProgram(0);
}
