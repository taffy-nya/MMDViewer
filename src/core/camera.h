#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    Camera() { update_camera_vectors(); }
    ~Camera() = default;

    glm::mat4 get_view_matrix() const;
    glm::mat4 get_projection_matrix() const;

    void reset();
    void resize(int width, int height);
    void update_camera_vectors();

    void handle_scroll(double yoffset);
    void handle_keys(int key, int action);

    void orbit(float delta_x, float delta_y);
    void pan(float delta_x, float delta_y);

    glm::vec3 position{0.0f, 10.0f, 35.0f};
    glm::vec3 target{0.0f, 10.0f, 0.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 right{};
    glm::vec3 world_up{0.0f, 1.0f, 0.0f};

    float yaw{-90.0f};
    float pitch{0.0f};

    float movement_speed{0.5f};
    float mouse_sensitivity{0.1f};
    float zoom{45.0f};

    float fov{45.0f};
    float aspect{1.0f};
    float z_near{0.1f};
    float z_far{1000.0f};
};
