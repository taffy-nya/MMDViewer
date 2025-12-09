#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Angel.h"

class Camera
{
public:
    Camera();
    ~Camera();

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();

    void updateCamera();
    void reset();

    // Interaction methods
    void handleMouseDrag(double deltaX, double deltaY, bool leftButton, bool rightButton, bool middleButton);
    void handleScroll(double yoffset);
    void handleKeys(int key, int action);
    void updateCameraVectors();

    // Camera parameters
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Euler Angles
    float yaw;
    float pitch;

    // Camera options
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    // Orbit/Model control (keeping the style of the original MMDViewer)
    glm::vec3 rotateTheta;
    glm::vec3 translateTheta;
    glm::vec3 scaleTheta;
    
    // Projection parameters
    float fov;
    float aspect;
    float zNear;
    float zFar;

};
#endif
