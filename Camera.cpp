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

void Camera::orbit(float deltaX, float deltaY) {
    glm::vec3 target(0.0f, 10.0f, 0.0f);
    
    // Current vector from target to camera
    glm::vec3 offset = position - target;
    
    // Rotate around Y axis (Yaw)
    // Note: deltaX is usually screen space, so dragging left (negative) should rotate camera right (positive angle) or vice versa.
    // Let's try standard orbit: drag left -> camera moves left -> view rotates right.
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-deltaX), glm::vec3(0.0f, 1.0f, 0.0f));
    offset = glm::vec3(rotation * glm::vec4(offset, 1.0f));
    
    // Rotate around Right axis (Pitch)
    // We need the current Right axis
    // front is (target - position), so right is cross(front, world_up)
    // But we can just use the current 'right' vector if it's up to date.
    // Let's recalculate to be safe.
    glm::vec3 currentFront = glm::normalize(target - position);
    glm::vec3 currentRight = glm::normalize(glm::cross(currentFront, world_up));
    
    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-deltaY), currentRight);
    offset = glm::vec3(rotation * glm::vec4(offset, 1.0f));
    
    position = target + offset;
    
    // Update Front to look at target
    front = glm::normalize(target - position);
    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::pan(float deltaX, float deltaY) {
    position -= right * deltaX * 0.5f;
    position += up * deltaY * 0.5f; 
}
