#ifndef _MESH_PAINTER_H_
#define _MESH_PAINTER_H_

#include "TriMesh.h"
#include "Camera.h"
#include <vector>
#include <string>

struct ShaderConfig {
    std::string vshader;
    std::string fshader;
    std::string vshader_edge;
    std::string fshader_edge;

    static ShaderConfig default_mmd() {
        return {
            "shaders/vshader.glsl",
            "shaders/fshader.glsl",
            "shaders/vshader_edge.glsl",
            "shaders/fshader_edge.glsl"
        };
    }
};

struct OpenGLObject {
    GLuint vao = 0, vbo = 0, ebo = 0, program = 0;
    
    // Uniform locations
    GLuint model_location, view_location, projection_location;
    GLuint object_color_location, light_color_location, light_pos_location, view_pos_location;
    GLuint has_texture_location, shininess_location;
    GLuint edge_size_location, edge_color_location; // For edge shader
    GLuint texture_sampler_location, toon_sampler_location;
    GLuint bone_matrices_location;
    
    // TBO for bones
    GLuint bone_tbo = 0;
    GLuint bone_texture = 0;
};

class MeshPainter
{
public:
    MeshPainter();
    ~MeshPainter();

    void add_mesh(TriMesh* mesh, const ShaderConfig& config = ShaderConfig::default_mmd());
    void draw_meshes(Camera* camera, const glm::vec3& lightPos);
    void clean_up();

private:
    struct MeshEntry {
        TriMesh* mesh;
        OpenGLObject main_object;
        OpenGLObject edge_object;
    };

    std::vector<MeshEntry> meshes;

    void bind_object_and_data(TriMesh* mesh, OpenGLObject& object, const std::string& vshader, const std::string& fshader, bool isEdge);
};

#endif
