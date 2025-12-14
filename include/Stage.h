#ifndef _STAGE_H_
#define _STAGE_H_

#include "Angel.h"
#include "Camera.h"

class Stage {
public:
    Stage(float size = 50.0f, int divisions = 20);
    ~Stage();
    void draw(Camera* camera);

private:
    GLuint vao_lines, vbo_lines;
    GLuint vao_plane, vbo_plane;
    GLuint program;
    GLuint model_loc, view_loc, proj_loc, color_loc;
    int line_vertex_count;
};

#endif
