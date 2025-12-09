#ifndef _MESH_PAINTER_H_
#define _MESH_PAINTER_H_

#include "TriMesh.h"
#include "Camera.h"
#include <vector>
#include <string>

struct OpenGLObject {
    GLuint vao = 0, vbo = 0, ebo = 0, program = 0;
    
    // Uniform locations
    GLuint modelLocation, viewLocation, projectionLocation;
    GLuint objectColorLocation, lightColorLocation, lightPosLocation, viewPosLocation;
    GLuint brightnessLocation, hasTextureLocation, shininessLocation;
    GLuint edgeSizeLocation, edgeColorLocation; // For edge shader
    GLuint textureSamplerLocation, toonSamplerLocation;
};

class MeshPainter
{
public:
    MeshPainter();
    ~MeshPainter();

    void addMesh(TriMesh* mesh, const std::string& vshader, const std::string& fshader, const std::string& vshader_edge, const std::string& fshader_edge);
    void drawMeshes(Camera* camera, const glm::vec3& lightPos, float brightness);
    void cleanUp();

private:
    struct MeshEntry {
        TriMesh* mesh;
        OpenGLObject mainObject;
        OpenGLObject edgeObject;
    };

    std::vector<MeshEntry> meshes;

    void bindObjectAndData(TriMesh* mesh, OpenGLObject& object, const std::string& vshader, const std::string& fshader, bool isEdge);
};

#endif
