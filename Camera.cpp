#include "Camera.h"
#include <iostream>

Camera::Camera()
    : position(glm::vec3(0.0f, 10.0f, 35.0f)),
      front(glm::vec3(0.0f, 0.0f, -1.0f)),
      up(glm::vec3(0.0f, 1.0f, 0.0f)),
      worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
      yaw(-90.0f),
      pitch(0.0f),
      movementSpeed(0.5f),
      mouseSensitivity(0.1f),
      zoom(45.0f),
      rotateTheta(0.0f),
      translateTheta(0.0f),
      scaleTheta(1.0f),
      fov(45.0f),
      aspect(1.0f),
      zNear(0.1f),
      zFar(1000.0f) {
    updateCameraVectors();
}

Camera::~Camera() {}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() {
    return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::handleKeys(int key, int action) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_W) position += movementSpeed * front;
        if (key == GLFW_KEY_S) position -= movementSpeed * front;
        if (key == GLFW_KEY_A) position -= glm::normalize(glm::cross(front, up)) * movementSpeed;
        if (key == GLFW_KEY_D) position += glm::normalize(glm::cross(front, up)) * movementSpeed;
    }
}

void Camera::handleScroll(double yoffset) {
    // Zooming by moving camera forward/backward
    position += (float)yoffset * front * 1.0f; 
}

void Camera::reset() {
    position = glm::vec3(0.0f, 10.0f, 35.0f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw = -90.0f;
    pitch = 0.0f;
    updateCameraVectors();
}

// Note: Mouse drag for model rotation is handled outside or we can add it here if we want Camera to manage "Orbit" style
// For now, we keep the WASD free-cam style from the original code's key_callback
