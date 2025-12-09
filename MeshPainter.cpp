#include "MeshPainter.h"
#include "Angel.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

MeshPainter::MeshPainter() {}
MeshPainter::~MeshPainter() { cleanUp(); }

void MeshPainter::cleanUp() {
    for (auto& entry : meshes) {
        glDeleteVertexArrays(1, &entry.mainObject.vao);
        glDeleteBuffers(1, &entry.mainObject.vbo);
        glDeleteBuffers(1, &entry.mainObject.ebo);
        glDeleteProgram(entry.mainObject.program);
        
        if (entry.edgeObject.program != 0) {
            glDeleteProgram(entry.edgeObject.program);
        }
    }
    meshes.clear();
}

void MeshPainter::addMesh(TriMesh* mesh, const std::string& vshader, const std::string& fshader, const std::string& vshader_edge, const std::string& fshader_edge) {
    MeshEntry entry;
    entry.mesh = mesh;
    
    // Bind Main Object
    bindObjectAndData(mesh, entry.mainObject, vshader, fshader, false);
    
    // Cache Uniform Locations for Main Object
    entry.mainObject.modelLocation = glGetUniformLocation(entry.mainObject.program, "model");
    entry.mainObject.viewLocation = glGetUniformLocation(entry.mainObject.program, "view");
    entry.mainObject.projectionLocation = glGetUniformLocation(entry.mainObject.program, "projection");
    entry.mainObject.objectColorLocation = glGetUniformLocation(entry.mainObject.program, "objectColor");
    entry.mainObject.lightColorLocation = glGetUniformLocation(entry.mainObject.program, "lightColor");
    entry.mainObject.lightPosLocation = glGetUniformLocation(entry.mainObject.program, "lightPos");
    entry.mainObject.viewPosLocation = glGetUniformLocation(entry.mainObject.program, "viewPos");
    entry.mainObject.brightnessLocation = glGetUniformLocation(entry.mainObject.program, "brightness");
    entry.mainObject.hasTextureLocation = glGetUniformLocation(entry.mainObject.program, "hasTexture");
    entry.mainObject.shininessLocation = glGetUniformLocation(entry.mainObject.program, "shininess");
    entry.mainObject.textureSamplerLocation = glGetUniformLocation(entry.mainObject.program, "textureSampler");
    entry.mainObject.toonSamplerLocation = glGetUniformLocation(entry.mainObject.program, "toonSampler");

    
    entry.edgeObject.program = InitShader(vshader_edge.c_str(), fshader_edge.c_str());
    entry.edgeObject.modelLocation = glGetUniformLocation(entry.edgeObject.program, "model");
    entry.edgeObject.viewLocation = glGetUniformLocation(entry.edgeObject.program, "view");
    entry.edgeObject.projectionLocation = glGetUniformLocation(entry.edgeObject.program, "projection");
    entry.edgeObject.edgeSizeLocation = glGetUniformLocation(entry.edgeObject.program, "edge_size");
    entry.edgeObject.edgeColorLocation = glGetUniformLocation(entry.edgeObject.program, "edge_color");
    
    // We share VAO for edge pass
    entry.edgeObject.vao = entry.mainObject.vao; 

    meshes.push_back(entry);
}

void MeshPainter::bindObjectAndData(TriMesh* mesh, OpenGLObject& object, const std::string& vshader, const std::string& fshader, bool isEdge) {
    glGenVertexArrays(1, &object.vao);
    glBindVertexArray(object.vao);
    
    const auto &p = mesh->getVertexPositions();
    const auto &n = mesh->getVertexNormals();
    const auto &u = mesh->getVertexUVs();
    
    if (p.empty() || p.size() != n.size() || p.size() != u.size()) { 
        std::cerr << "Vertex attribute mismatch!" << std::endl; 
        return; 
    }
    
    size_t p_size = p.size()*sizeof(glm::vec3);
    size_t n_size = n.size()*sizeof(glm::vec3);
    size_t u_size = u.size()*sizeof(glm::vec2);
    
    glGenBuffers(1, &object.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
    glBufferData(GL_ARRAY_BUFFER, p_size+n_size+u_size, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, p_size, p.data());
    glBufferSubData(GL_ARRAY_BUFFER, p_size, n_size, n.data());
    glBufferSubData(GL_ARRAY_BUFFER, p_size+n_size, u_size, u.data());
    
    glGenBuffers(1, &object.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->getFaces().size()*sizeof(vec3u), mesh->getFaces().data(), GL_STATIC_DRAW);
    
    object.program = InitShader(vshader.c_str(), fshader.c_str());
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(p_size)); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(p_size+n_size)); glEnableVertexAttribArray(2);
}

void MeshPainter::drawMeshes(Camera* camera, const glm::vec3& lightPos, float brightness) {
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = camera->getProjectionMatrix();
    glm::vec3 viewPos = camera->position;

    for (const auto& entry : meshes) {
        glm::mat4 model = entry.mesh->getModelMatrix();
        
        glBindVertexArray(entry.mainObject.vao);

        // Pass 1: Outline
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glUseProgram(entry.edgeObject.program);
        
        glUniformMatrix4fv(entry.edgeObject.modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(entry.edgeObject.viewLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(entry.edgeObject.projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
        
        unsigned int face_offset = 0;
        for (const auto& mat : entry.mesh->getMaterials()) {
            glUniform1f(entry.edgeObject.edgeSizeLocation, mat.edge_size * 0.03f);
            glUniform4fv(entry.edgeObject.edgeColorLocation, 1, glm::value_ptr(mat.edge_color));
            glDrawElements(GL_TRIANGLES, mat.num_faces, GL_UNSIGNED_INT, (void*)(face_offset*sizeof(unsigned int)));
            face_offset += mat.num_faces;
        }

        // Pass 2: Main Model
        glCullFace(GL_BACK);
        glUseProgram(entry.mainObject.program);
        
        glUniformMatrix4fv(entry.mainObject.modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(entry.mainObject.viewLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(entry.mainObject.projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform3fv(entry.mainObject.lightColorLocation, 1, glm::value_ptr(glm::vec3(1.0f)));
        glUniform3fv(entry.mainObject.lightPosLocation, 1, glm::value_ptr(lightPos));
        glUniform3fv(entry.mainObject.viewPosLocation, 1, glm::value_ptr(viewPos));
        glUniform1f(entry.mainObject.brightnessLocation, brightness);
        
        glUniform1i(entry.mainObject.textureSamplerLocation, 0);
        glUniform1i(entry.mainObject.toonSamplerLocation, 1);
        
        face_offset = 0;
        for (const auto& mat : entry.mesh->getMaterials()) {
            glUniform4fv(entry.mainObject.objectColorLocation, 1, glm::value_ptr(mat.diffuse_color));
            glUniform1f(entry.mainObject.shininessLocation, mat.shininess);
            
            if (mat.has_texture) {
                glUniform1i(entry.mainObject.hasTextureLocation, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, entry.mesh->getTextures()[mat.texture_index].gl_texture_id);
            } else {
                glUniform1i(entry.mainObject.hasTextureLocation, 0);
            }
            
            glDrawElements(GL_TRIANGLES, mat.num_faces, GL_UNSIGNED_INT, (void*)(face_offset*sizeof(unsigned int)));
            face_offset += mat.num_faces;
        }
        
        glDisable(GL_CULL_FACE);
    }
    glBindVertexArray(0);
}
