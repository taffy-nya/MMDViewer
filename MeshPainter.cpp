#include "MeshPainter.h"
#include "Angel.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

MeshPainter::MeshPainter() {}
MeshPainter::~MeshPainter() { clean_up(); }

void MeshPainter::clean_up() {
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

void MeshPainter::add_mesh(TriMesh* mesh, const std::string& vshader, const std::string& fshader, const std::string& vshader_edge, const std::string& fshader_edge) {
    MeshEntry entry;
    entry.mesh = mesh;
    
    // Bind Main Object
    bind_object_and_data(mesh, entry.main_object, vshader, fshader, false);
    
    // Cache Uniform Locations for Main Object
    entry.main_object.model_location = glGetUniformLocation(entry.main_object.program, "model");
    entry.main_object.view_location = glGetUniformLocation(entry.main_object.program, "view");
    entry.main_object.projection_location = glGetUniformLocation(entry.main_object.program, "projection");
    entry.main_object.object_color_location = glGetUniformLocation(entry.main_object.program, "objectColor");
    entry.main_object.light_color_location = glGetUniformLocation(entry.main_object.program, "lightColor");
    entry.main_object.light_pos_location = glGetUniformLocation(entry.main_object.program, "lightPos");
    entry.main_object.view_pos_location = glGetUniformLocation(entry.main_object.program, "viewPos");
    entry.main_object.has_texture_location = glGetUniformLocation(entry.main_object.program, "hasTexture");
    entry.main_object.shininess_location = glGetUniformLocation(entry.main_object.program, "shininess");
    entry.main_object.texture_sampler_location = glGetUniformLocation(entry.main_object.program, "textureSampler");
    entry.main_object.toon_sampler_location = glGetUniformLocation(entry.main_object.program, "toonSampler");
    entry.main_object.bone_matrices_location = glGetUniformLocation(entry.main_object.program, "boneMatrices");

    
    entry.edge_object.program = InitShader(vshader_edge.c_str(), fshader_edge.c_str());
    entry.edge_object.model_location = glGetUniformLocation(entry.edge_object.program, "model");
    entry.edge_object.view_location = glGetUniformLocation(entry.edge_object.program, "view");
    entry.edge_object.projection_location = glGetUniformLocation(entry.edge_object.program, "projection");
    entry.edge_object.edge_size_location = glGetUniformLocation(entry.edge_object.program, "edge_size");
    entry.edge_object.edge_color_location = glGetUniformLocation(entry.edge_object.program, "edge_color");
    entry.edge_object.bone_matrices_location = glGetUniformLocation(entry.edge_object.program, "boneMatrices");
    
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

void MeshPainter::draw_meshes(Camera* camera, const glm::vec3& lightPos) {
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
        
        glUniform3fv(entry.main_object.light_color_location, 1, glm::value_ptr(glm::vec3(1.0f)));
        glUniform3fv(entry.main_object.light_pos_location, 1, glm::value_ptr(lightPos));
        glUniform3fv(entry.main_object.view_pos_location, 1, glm::value_ptr(viewPos));
        
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
