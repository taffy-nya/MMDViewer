#include "Camera.h"
#include <iostream>

Camera::Camera()
    : position(glm::vec3(0.0f, 10.0f, 35.0f)),
      target(glm::vec3(0.0f, 10.0f, 0.0f)),
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
        glm::vec3 delta(0.0f);
        if (key == GLFW_KEY_W) delta += movement_speed * front;
        if (key == GLFW_KEY_S) delta -= movement_speed * front;
        if (key == GLFW_KEY_A) delta -= glm::normalize(glm::cross(front, up)) * movement_speed;
        if (key == GLFW_KEY_D) delta += glm::normalize(glm::cross(front, up)) * movement_speed;
        
        position += delta;
        target += delta;
    }
}

void Camera::handle_scroll(double yoffset) {
    // Zooming by moving camera forward/backward
    position += (float)yoffset * front * 1.0f; 
}

void Camera::reset() {
    position = glm::vec3(0.0f, 10.0f, 35.0f);
    target = glm::vec3(0.0f, 10.0f, 0.0f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw = -90.0f;
    pitch = 0.0f;
    update_camera_vectors();
}

// Note: Mouse drag for model rotation is handled outside or we can add it here if we want Camera to manage "Orbit" style
// For now, we keep the WASD free-cam style from the original code's key_callback

void Camera::orbit(float deltaX, float deltaY) {
    // Current vector from target to camera
    glm::vec3 offset = position - target;
    
    // Rotate around World Up (Yaw)
    // We rotate the offset, and also the camera's Up and Right vectors to maintain orientation
    glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), glm::radians(-deltaX), glm::vec3(0.0f, 1.0f, 0.0f));
    offset = glm::vec3(yawRot * glm::vec4(offset, 1.0f));
    up = glm::vec3(yawRot * glm::vec4(up, 0.0f));
    right = glm::vec3(yawRot * glm::vec4(right, 0.0f));

    // Rotate around Camera Right (Pitch)
    // This allows going over the top (up vector will flip naturally)
    glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), glm::radians(-deltaY), right);
    offset = glm::vec3(pitchRot * glm::vec4(offset, 1.0f));
    up = glm::vec3(pitchRot * glm::vec4(up, 0.0f));
    
    position = target + offset;
    
    // Update Front to look at target
    front = glm::normalize(target - position);
    
    // Re-orthogonalize to prevent drift
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::pan(float deltaX, float deltaY) {
    glm::vec3 offset = -right * deltaX * 0.5f + up * deltaY * 0.5f;
    position += offset;
    target += offset;
}
