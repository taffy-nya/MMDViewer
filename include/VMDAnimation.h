#ifndef _VMD_ANIMATION_H_
#define _VMD_ANIMATION_H_

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class TriMesh;

struct BoneKeyframe {
    int frame;
    glm::vec3 position;
    glm::quat rotation;
};

struct AnimationTrack {
    std::vector<BoneKeyframe> keyframes;
};

class VMDAnimation {
public:
    VMDAnimation();
    ~VMDAnimation();

    bool load(const std::string& filename);
    void update(float frame, TriMesh* mesh);
    float getDuration() const { return duration; }

private:
    std::map<std::string, AnimationTrack> tracks;
    float duration = 0.0f;
};

#endif
