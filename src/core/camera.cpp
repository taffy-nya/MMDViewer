#include "camera.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::get_view_matrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::get_projection_matrix() const {
    return glm::perspective(glm::radians(fov), aspect, z_near, z_far);
}

void Camera::update_camera_vectors() {
    glm::vec3 new_front;
    new_front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    new_front.y = sin(glm::radians(pitch));
    new_front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(new_front);

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
    position += static_cast<float>(yoffset) * front * 1.0f;
}

void Camera::reset() {
    position = {0.0f, 10.0f, 35.0f};
    target   = {0.0f, 10.0f, 0.0f};
    front    = {0.0f, 0.0f, -1.0f};
    up       = {0.0f, 1.0f, 0.0f};
    yaw      = -90.0f;
    pitch    = 0.0f;
    update_camera_vectors();
}

void Camera::resize(int width, int height) {
    aspect = static_cast<float>(width) / static_cast<float>(height);
}

void Camera::orbit(float delta_x, float delta_y) {
    glm::vec3 offset = position - target;

    glm::mat4 yaw_rot = glm::rotate(glm::mat4(1.0f), glm::radians(-delta_x), glm::vec3(0.0f, 1.0f, 0.0f));
    offset = glm::vec3(yaw_rot * glm::vec4(offset, 1.0f));
    up = glm::vec3(yaw_rot * glm::vec4(up, 0.0f));
    right = glm::vec3(yaw_rot * glm::vec4(right, 0.0f));

    glm::mat4 pitch_rot = glm::rotate(glm::mat4(1.0f), glm::radians(-delta_y), right);
    offset = glm::vec3(pitch_rot * glm::vec4(offset, 1.0f));
    up = glm::vec3(pitch_rot * glm::vec4(up, 0.0f));

    position = target + offset;

    front = glm::normalize(target - position);

    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::pan(float delta_x, float delta_y) {
    glm::vec3 offset = -right * delta_x * 0.5f + up * delta_y * 0.5f;
    position += offset;
    target += offset;
}
