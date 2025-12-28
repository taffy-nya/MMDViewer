#ifndef _VMD_ANIMATION_H_
#define _VMD_ANIMATION_H_

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class TriMesh;

// 骨骼关键帧
struct BoneKeyframe {
    int frame;           // 帧号
    glm::vec3 position;  // 相对位移 (相对于骨骼初始位置)
    glm::quat rotation;  // 旋转四元数
    // VMD 还包含贝塞尔曲线插值参数，这里没有实现
};

// 单个骨骼的所有关键帧
struct AnimationTrack {
    std::vector<BoneKeyframe> keyframes; // 按帧号排序
};

class VMDAnimation {
public:
    VMDAnimation();
    ~VMDAnimation();

    // 加载 VMD 动作文件
    bool load(const std::string& filename);
    
    // 更新模型姿态
    // frame 为当前播放的帧数, 这里 float 可以是小数, 用于插值
    void update(float frame, TriMesh* mesh);
    
    // 获取动画总长度 (即最后一帧的帧号)
    float get_duration() const { return duration; }

private:
    // 骨骼名称 -> 动画轨道
    std::map<std::string, AnimationTrack> tracks;
    float duration = 0.0f;
};

#endif
