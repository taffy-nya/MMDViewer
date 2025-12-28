#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Angel.h"

class Camera
{
public:
    Camera();
    ~Camera();

    // 获取视图矩阵
    glm::mat4 get_view_matrix();

    // 获取投影矩阵
    glm::mat4 get_projection_matrix();

    void update_camera();
    void reset();

    // 键盘鼠标交互
    void handle_mouse_drag(double deltaX, double deltaY, bool leftButton, bool rightButton, bool middleButton);
    void handle_scroll(double yoffset);
    void handle_keys(int key, int action);
    void update_camera_vectors();
    
    // 围绕目标点旋转相机
    void orbit(float deltaX, float deltaY);
    // 平移相机
    void pan(float deltaX, float deltaY);

    // 相机参数
    glm::vec3 position;
    glm::vec3 target; // 轨道控制的中心点
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    // 球坐标下角度
    float yaw;
    float pitch;

    // 相机选项
    float movement_speed;
    float mouse_sensitivity;
    float zoom;

    // 轨道/模型控制
    glm::vec3 rotate_theta;
    glm::vec3 translate_theta;
    glm::vec3 scale_theta;
    
    // 投影参数
    float fov;    // 视野角度
    float aspect; // 宽高比
    float z_near; // 近裁剪面
    float z_far;  // 远裁剪面

};
#endif
