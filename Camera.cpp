#include "Camera.h"
#include <iostream>

Camera::Camera()
    : position(glm::vec3(0.0f, 10.0f, 35.0f)),
      front(glm::vec3(0.0f, 0.0f, -1.0f)),
      up(glm::vec3(0.0f, 1.0f, 0.0f)),
      world_up(glm::vec3(0.0f, 1.0f, 0.0f)),
      yaw(-90.0f),
      pitch(0.0f),
      movement_speed(0.5f),
      mouse_sensitivity(0.1f),
      zoom(45.0f),
      rotate_theta(0.0f),
      translate_theta(0.0f),
      scale_theta(1.0f),
      fov(45.0f),
      aspect(1.0f),
      z_near(0.1f),
      z_far(1000.0f) {
    update_camera_vectors();
}

Camera::~Camera() {}

glm::mat4 Camera::get_view_matrix() {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::get_projection_matrix() {
    return glm::perspective(glm::radians(fov), aspect, z_near, z_far);
}

void Camera::update_camera_vectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::handle_keys(int key, int action) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_W) position += movement_speed * front;
        if (key == GLFW_KEY_S) position -= movement_speed * front;
        if (key == GLFW_KEY_A) position -= glm::normalize(glm::cross(front, up)) * movement_speed;
        if (key == GLFW_KEY_D) position += glm::normalize(glm::cross(front, up)) * movement_speed;
    }
}

void Camera::handle_scroll(double yoffset) {
    // Zooming by moving camera forward/backward
    position += (float)yoffset * front * 1.0f; 
}

void Camera::reset() {
    position = glm::vec3(0.0f, 10.0f, 35.0f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw = -90.0f;
    pitch = 0.0f;
    update_camera_vectors();
}

// Note: Mouse drag for model rotation is handled outside or we can add it here if we want Camera to manage "Orbit" style
// For now, we keep the WASD free-cam style from the original code's key_callback
