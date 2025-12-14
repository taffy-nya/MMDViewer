#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Angel.h"

class Camera
{
public:
    Camera();
    ~Camera();

    glm::mat4 get_view_matrix();
    glm::mat4 get_projection_matrix();

    void update_camera();
    void reset();

    // Interaction methods
    void handle_mouse_drag(double deltaX, double deltaY, bool leftButton, bool rightButton, bool middleButton);
    void handle_scroll(double yoffset);
    void handle_keys(int key, int action);
    void update_camera_vectors();
    
    // Orbit control
    void orbit(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY);

    // Camera parameters
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    // Euler Angles
    float yaw;
    float pitch;

    // Camera options
    float movement_speed;
    float mouse_sensitivity;
    float zoom;

    // Orbit/Model control (keeping the style of the original MMDViewer)
    glm::vec3 rotate_theta;
    glm::vec3 translate_theta;
    glm::vec3 scale_theta;
    
    // Projection parameters
    float fov;
    float aspect;
    float z_near;
    float z_far;

};
#endif
