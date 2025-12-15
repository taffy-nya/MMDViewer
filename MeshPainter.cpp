#include "MeshPainter.h"
#include "Angel.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

MeshPainter::MeshPainter() {
    init_gizmo_resources();
}
MeshPainter::~MeshPainter() { clean_up(); }

void MeshPainter::clean_up() {
    if (gizmo_vao != 0) glDeleteVertexArrays(1, &gizmo_vao);
    if (gizmo_vbo != 0) glDeleteBuffers(1, &gizmo_vbo);
    if (gizmo_program != 0) glDeleteProgram(gizmo_program);

    for (auto& entry : meshes) {
        glDeleteVertexArrays(1, &entry.main_object.vao);
        glDeleteBuffers(1, &entry.main_object.vbo);
        glDeleteBuffers(1, &entry.main_object.ebo);
        glDeleteProgram(entry.main_object.program);
        
        if (entry.edge_object.program != 0) {
            glDeleteProgram(entry.edge_object.program);
        }
        
        if (entry.main_object.bone_tbo != 0) glDeleteBuffers(1, &entry.main_object.bone_tbo);
        if (entry.main_object.bone_texture != 0) glDeleteTextures(1, &entry.main_object.bone_texture);
    }
    meshes.clear();
}

void MeshPainter::add_mesh(TriMesh* mesh, const ShaderConfig& config) {
    MeshEntry entry;
    entry.mesh = mesh;
    
    // Bind Main Object
    bind_object_and_data(mesh, entry.main_object, config.vshader, config.fshader, false);
    
    // Cache Uniform Locations for Main Object
    entry.main_object.model_location = glGetUniformLocation(entry.main_object.program, "model");
    entry.main_object.view_location = glGetUniformLocation(entry.main_object.program, "view");
    entry.main_object.projection_location = glGetUniformLocation(entry.main_object.program, "projection");
    entry.main_object.object_color_location = glGetUniformLocation(entry.main_object.program, "objectColor");
    entry.main_object.view_pos_location = glGetUniformLocation(entry.main_object.program, "viewPos");
    entry.main_object.has_texture_location = glGetUniformLocation(entry.main_object.program, "hasTexture");
    entry.main_object.shininess_location = glGetUniformLocation(entry.main_object.program, "shininess");
    entry.main_object.texture_sampler_location = glGetUniformLocation(entry.main_object.program, "textureSampler");
    entry.main_object.toon_sampler_location = glGetUniformLocation(entry.main_object.program, "toonSampler");
    entry.main_object.bone_matrices_location = glGetUniformLocation(entry.main_object.program, "boneMatrices");

    // Light Uniforms
    entry.main_object.ambient_color_location = glGetUniformLocation(entry.main_object.program, "ambientColor");
    entry.main_object.ambient_strength_location = glGetUniformLocation(entry.main_object.program, "ambientStrength");
    entry.main_object.num_lights_location = glGetUniformLocation(entry.main_object.program, "numLights");
    entry.main_object.brightness_location = glGetUniformLocation(entry.main_object.program, "brightness");
    
    // Shadow Uniforms
    entry.main_object.shadowMap_location = glGetUniformLocation(entry.main_object.program, "shadowMap");
    entry.main_object.lightSpaceMatrix_location = glGetUniformLocation(entry.main_object.program, "lightSpaceMatrix");

    // Shadow Pass Program
    entry.main_object.shadow_program = InitShader("shaders/vshader_shadow.glsl", "shaders/fshader_shadow.glsl");
    entry.main_object.shadow_model_location = glGetUniformLocation(entry.main_object.shadow_program, "model");
    entry.main_object.shadow_lightSpaceMatrix_location = glGetUniformLocation(entry.main_object.shadow_program, "lightSpaceMatrix");
    entry.main_object.shadow_boneMatrices_location = glGetUniformLocation(entry.main_object.shadow_program, "boneMatrices");
    
    for (int i = 0; i < 16; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        entry.main_object.light_locations[i].position = glGetUniformLocation(entry.main_object.program, (base + ".position").c_str());
        entry.main_object.light_locations[i].direction = glGetUniformLocation(entry.main_object.program, (base + ".direction").c_str());
        entry.main_object.light_locations[i].color = glGetUniformLocation(entry.main_object.program, (base + ".color").c_str());
        entry.main_object.light_locations[i].intensity = glGetUniformLocation(entry.main_object.program, (base + ".intensity").c_str());
        entry.main_object.light_locations[i].type = glGetUniformLocation(entry.main_object.program, (base + ".type").c_str());
        entry.main_object.light_locations[i].constant = glGetUniformLocation(entry.main_object.program, (base + ".constant").c_str());
        entry.main_object.light_locations[i].linear = glGetUniformLocation(entry.main_object.program, (base + ".linear").c_str());
        entry.main_object.light_locations[i].quadratic = glGetUniformLocation(entry.main_object.program, (base + ".quadratic").c_str());
        entry.main_object.light_locations[i].enabled = glGetUniformLocation(entry.main_object.program, (base + ".enabled").c_str());
    }
    
    entry.edge_object.program = InitShader(config.vshader_edge.c_str(), config.fshader_edge.c_str());
    entry.edge_object.model_location = glGetUniformLocation(entry.edge_object.program, "model");
    entry.edge_object.view_location = glGetUniformLocation(entry.edge_object.program, "view");
    entry.edge_object.projection_location = glGetUniformLocation(entry.edge_object.program, "projection");
    entry.edge_object.edge_size_location = glGetUniformLocation(entry.edge_object.program, "edge_size");
    entry.edge_object.edge_color_location = glGetUniformLocation(entry.edge_object.program, "edge_color");
    entry.edge_object.bone_matrices_location = glGetUniformLocation(entry.edge_object.program, "boneMatrices");
    
    // Edge Light Uniforms
    entry.edge_object.ambient_color_location = glGetUniformLocation(entry.edge_object.program, "ambientColor");
    entry.edge_object.ambient_strength_location = glGetUniformLocation(entry.edge_object.program, "ambientStrength");
    entry.edge_object.num_lights_location = glGetUniformLocation(entry.edge_object.program, "numLights");
    
    for (int i = 0; i < 16; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        entry.edge_object.light_locations[i].position = glGetUniformLocation(entry.edge_object.program, (base + ".position").c_str());
        entry.edge_object.light_locations[i].direction = glGetUniformLocation(entry.edge_object.program, (base + ".direction").c_str());
        entry.edge_object.light_locations[i].color = glGetUniformLocation(entry.edge_object.program, (base + ".color").c_str());
        entry.edge_object.light_locations[i].intensity = glGetUniformLocation(entry.edge_object.program, (base + ".intensity").c_str());
        entry.edge_object.light_locations[i].type = glGetUniformLocation(entry.edge_object.program, (base + ".type").c_str());
        entry.edge_object.light_locations[i].constant = glGetUniformLocation(entry.edge_object.program, (base + ".constant").c_str());
        entry.edge_object.light_locations[i].linear = glGetUniformLocation(entry.edge_object.program, (base + ".linear").c_str());
        entry.edge_object.light_locations[i].quadratic = glGetUniformLocation(entry.edge_object.program, (base + ".quadratic").c_str());
        entry.edge_object.light_locations[i].enabled = glGetUniformLocation(entry.edge_object.program, (base + ".enabled").c_str());
    }

    // We share VAO for edge pass
    entry.edge_object.vao = entry.main_object.vao; 

    meshes.push_back(entry);
}

void MeshPainter::bind_object_and_data(TriMesh* mesh, OpenGLObject& object, const std::string& vshader, const std::string& fshader, bool isEdge) {
    glGenVertexArrays(1, &object.vao);
    glBindVertexArray(object.vao);
    
    const auto &p = mesh->get_vertex_positions();
    const auto &n = mesh->get_vertex_normals();
    const auto &u = mesh->get_vertex_uvs();
    const auto &b = mesh->get_vertex_bone_data();
    
    if (p.empty() || p.size() != n.size() || p.size() != u.size() || p.size() != b.size()) { 
        std::cerr << "Vertex attribute mismatch!" << std::endl; 
        return; 
    }
    
    size_t p_size = p.size()*sizeof(glm::vec3);
    size_t n_size = n.size()*sizeof(glm::vec3);
    size_t u_size = u.size()*sizeof(glm::vec2);
    size_t b_size = b.size()*sizeof(VertexBoneData);
    
    glGenBuffers(1, &object.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
    glBufferData(GL_ARRAY_BUFFER, p_size+n_size+u_size+b_size, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, p_size, p.data());
    glBufferSubData(GL_ARRAY_BUFFER, p_size, n_size, n.data());
    glBufferSubData(GL_ARRAY_BUFFER, p_size+n_size, u_size, u.data());
    glBufferSubData(GL_ARRAY_BUFFER, p_size+n_size+u_size, b_size, b.data());
    
    glGenBuffers(1, &object.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->get_faces().size()*sizeof(vec3u), mesh->get_faces().data(), GL_STATIC_DRAW);
    
    object.program = InitShader(vshader.c_str(), fshader.c_str());
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(p_size)); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(p_size+n_size)); glEnableVertexAttribArray(2);

    // Bone Indices (ivec4)
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(VertexBoneData), BUFFER_OFFSET(p_size+n_size+u_size)); 
    glEnableVertexAttribArray(3);
    
    // Bone Weights (vec4)
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), BUFFER_OFFSET(p_size+n_size+u_size + sizeof(glm::ivec4))); 
    glEnableVertexAttribArray(4);

    // Initialize TBO for bones
    glGenBuffers(1, &object.bone_tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, object.bone_tbo);
    glGenTextures(1, &object.bone_texture);
}

void MeshPainter::draw_meshes(Camera* camera, const std::vector<Light>& lights, const glm::vec3& ambientColor, float ambientStrength, GLuint shadowMap, const glm::mat4& lightSpaceMatrix, float brightness) {
    glm::mat4 view = camera->get_view_matrix();
    glm::mat4 projection = camera->get_projection_matrix();
    glm::vec3 viewPos = camera->position;

    for (const auto& entry : meshes) {
        glm::mat4 model = entry.mesh->get_model_matrix();
        
        // Prepare bone matrices
        std::vector<glm::mat4> boneTransforms;
        auto& bones = entry.mesh->get_bones();
        if (!bones.empty()) {
            boneTransforms.reserve(bones.size());
            for (const auto& bone : bones) {
                boneTransforms.push_back(bone.global_transform * bone.offset_matrix);
            }
        }

        glBindVertexArray(entry.main_object.vao);

        // Pass 1: Outline
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glUseProgram(entry.edge_object.program);
        
        if (!boneTransforms.empty()) {
             // Update TBO
             glBindBuffer(GL_TEXTURE_BUFFER, entry.main_object.bone_tbo);
             glBufferData(GL_TEXTURE_BUFFER, boneTransforms.size() * sizeof(glm::mat4), boneTransforms.data(), GL_DYNAMIC_DRAW);
             
             // Bind TBO to Texture Unit 2
             glActiveTexture(GL_TEXTURE2);
             glBindTexture(GL_TEXTURE_BUFFER, entry.main_object.bone_texture);
             glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, entry.main_object.bone_tbo);
             
             glUniform1i(entry.edge_object.bone_matrices_location, 2);
        }
        
        glUniformMatrix4fv(entry.edge_object.model_location, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(entry.edge_object.view_location, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(entry.edge_object.projection_location, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Pass lights to edge shader
        glUniform3fv(entry.edge_object.ambient_color_location, 1, glm::value_ptr(ambientColor));
        glUniform1f(entry.edge_object.ambient_strength_location, ambientStrength);
        glUniform1i(entry.edge_object.num_lights_location, (int)lights.size());

        for (int i = 0; i < lights.size() && i < 16; ++i) {
            const auto& light = lights[i];
            const auto& locs = entry.edge_object.light_locations[i];
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

        unsigned int face_offset = 0;
        for (const auto& mat : entry.mesh->get_materials()) {
            glUniform1f(entry.edge_object.edge_size_location, mat.edge_size * 0.03f);
            glUniform4fv(entry.edge_object.edge_color_location, 1, glm::value_ptr(mat.edge_color));
            glDrawElements(GL_TRIANGLES, mat.num_faces, GL_UNSIGNED_INT, (void*)(face_offset*sizeof(unsigned int)));
            face_offset += mat.num_faces;
        }

        // Pass 2: Main Model
        glCullFace(GL_BACK);
        glUseProgram(entry.main_object.program);
        
        glUniformMatrix4fv(entry.main_object.lightSpaceMatrix_location, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glUniform1i(entry.main_object.shadowMap_location, 10);
        
        if (!boneTransforms.empty()) {
             // Bind TBO to Texture Unit 2 (already updated)
             glActiveTexture(GL_TEXTURE2);
             glBindTexture(GL_TEXTURE_BUFFER, entry.main_object.bone_texture);
             // glTexBuffer is associated with the texture object, so binding the texture is enough if we didn't change the buffer association
             // But to be safe/clear:
             // glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, entry.mainObject.boneTbo); 
             
             glUniform1i(entry.main_object.bone_matrices_location, 2);
        }
        
        glUniformMatrix4fv(entry.main_object.model_location, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(entry.main_object.view_location, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(entry.main_object.projection_location, 1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform3fv(entry.main_object.view_pos_location, 1, glm::value_ptr(viewPos));

        // Light Uniforms
        glUniform3fv(entry.main_object.ambient_color_location, 1, glm::value_ptr(ambientColor));
        glUniform1f(entry.main_object.ambient_strength_location, ambientStrength);
        // glUniform1f(entry.main_object.brightness_location, brightness); // Removed brightness uniform usage if not defined in struct
        glUniform1i(entry.main_object.num_lights_location, (int)lights.size());

        for (int i = 0; i < lights.size() && i < 16; ++i) {
            const auto& light = lights[i];
            const auto& locs = entry.main_object.light_locations[i];
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
        
        glUniform1i(entry.main_object.texture_sampler_location, 0);
        glUniform1i(entry.main_object.toon_sampler_location, 1);
        
        face_offset = 0;
        for (const auto& mat : entry.mesh->get_materials()) {
            glUniform4fv(entry.main_object.object_color_location, 1, glm::value_ptr(mat.diffuse_color));
            glUniform1f(entry.main_object.shininess_location, mat.shininess);
            
            if (mat.has_texture) {
                glUniform1i(entry.main_object.has_texture_location, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, entry.mesh->get_textures()[mat.texture_index].gl_texture_id);
            } else {
                glUniform1i(entry.main_object.has_texture_location, 0);
            }
            
            glDrawElements(GL_TRIANGLES, mat.num_faces, GL_UNSIGNED_INT, (void*)(face_offset*sizeof(unsigned int)));
            face_offset += mat.num_faces;
        }
        
        glDisable(GL_CULL_FACE);
    }
    glBindVertexArray(0);
}

void MeshPainter::draw_shadow(const glm::mat4& lightSpaceMatrix) {
    for (auto& entry : meshes) {
        glUseProgram(entry.main_object.shadow_program);
        
        glm::mat4 model = entry.mesh->get_model_matrix();
        glUniformMatrix4fv(entry.main_object.shadow_model_location, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(entry.main_object.shadow_lightSpaceMatrix_location, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        
        // Bind Bone Matrices
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, entry.main_object.bone_texture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, entry.main_object.bone_tbo);
        
        std::vector<glm::mat4> boneTransforms;
        auto& bones = entry.mesh->get_bones();
        if (!bones.empty()) {
            boneTransforms.reserve(bones.size());
            for (const auto& bone : bones) {
                boneTransforms.push_back(bone.global_transform * bone.offset_matrix);
            }
             glBufferData(GL_TEXTURE_BUFFER, boneTransforms.size() * sizeof(glm::mat4), boneTransforms.data(), GL_DYNAMIC_DRAW);
        }
        glUniform1i(entry.main_object.shadow_boneMatrices_location, 0);

        glBindVertexArray(entry.main_object.vao);
        
        // Draw Submeshes
        unsigned int face_offset = 0;
        for (const auto& mat : entry.mesh->get_materials()) {
             glDrawElements(GL_TRIANGLES, mat.num_faces, GL_UNSIGNED_INT, (void*)(face_offset * sizeof(unsigned int)));
             face_offset += mat.num_faces;
        }
        glBindVertexArray(0);
    }
}

void MeshPainter::init_gizmo_resources() {
    // Simple Cube Vertices
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
    };

    glGenVertexArrays(1, &gizmo_vao);
    glGenBuffers(1, &gizmo_vbo);

    glBindVertexArray(gizmo_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gizmo_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    gizmo_program = InitShader("shaders/vshader_color.glsl", "shaders/fshader_color.glsl");
    gizmo_model_loc = glGetUniformLocation(gizmo_program, "model");
    gizmo_view_loc = glGetUniformLocation(gizmo_program, "view");
    gizmo_proj_loc = glGetUniformLocation(gizmo_program, "projection");
    gizmo_color_loc = glGetUniformLocation(gizmo_program, "color");
}

void MeshPainter::draw_light_gizmos(Camera* camera, const std::vector<Light>& lights) {
    if (gizmo_program == 0) return;

    glUseProgram(gizmo_program);
    
    glm::mat4 view = camera->get_view_matrix();
    glm::mat4 projection = camera->get_projection_matrix();
    
    glUniformMatrix4fv(gizmo_view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(gizmo_proj_loc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(gizmo_vao);

    for (const auto& light : lights) {
        if (!light.enabled) continue;

        glm::mat4 model = glm::mat4(1.0f);
        if (light.type == LIGHT_POINT) {
            model = glm::translate(model, light.position);
            model = glm::scale(model, glm::vec3(0.5f));
        } else {
            // 暂不显示平行光源
            continue; 
        }

        glUniformMatrix4fv(gizmo_model_loc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(gizmo_color_loc, 1, glm::value_ptr(light.color));
        
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

