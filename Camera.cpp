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

// 计算视图矩阵
glm::mat4 Camera::get_view_matrix() {
    return glm::lookAt(position, position + front, up);
}

// 计算投影矩阵（透视投影）
glm::mat4 Camera::get_projection_matrix() {
    return glm::perspective(glm::radians(fov), aspect, z_near, z_far);
}

// 根据球坐标更新
void Camera::update_camera_vectors() {
    glm::vec3 newFront;
    // 将球坐标转换为笛卡尔坐标
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
    // 通过沿前向量移动相机来实现缩放效果
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

// 围绕目标点旋转相机
void Camera::orbit(float deltaX, float deltaY) {
    // 计算从目标点到相机的向量
    glm::vec3 offset = position - target;
    
    // 绕 y 轴旋转
    glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), glm::radians(-deltaX), glm::vec3(0.0f, 1.0f, 0.0f));
    offset = glm::vec3(yawRot * glm::vec4(offset, 1.0f));
    up = glm::vec3(yawRot * glm::vec4(up, 0.0f));
    right = glm::vec3(yawRot * glm::vec4(right, 0.0f));

    // 绕右向量旋转
    // 这样写 up 在越过头顶时也不会翻转
    glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), glm::radians(-deltaY), right);
    offset = glm::vec3(pitchRot * glm::vec4(offset, 1.0f));
    up = glm::vec3(pitchRot * glm::vec4(up, 0.0f));
    
    position = target + offset;
    
    // 更新前向量以指向目标
    front = glm::normalize(target - position);
    
    // 重新正交化以防止漂移
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::pan(float deltaX, float deltaY) {
    glm::vec3 offset = -right * deltaX * 0.5f + up * deltaY * 0.5f;
    position += offset;
    target += offset;
}
