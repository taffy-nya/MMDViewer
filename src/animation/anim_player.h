#pragma once
#include "core/anim.h"
#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

struct BoneDef;
class Skeleton;

class AnimPlayer {
public:
    AnimPlayer() = default;

    auto set_anim(const Animation& anim, const std::vector<BoneDef>& defs) -> std::expected<void, std::string>;

    void update(float frame, const std::vector<BoneDef>& defs, Skeleton& skeleton);

    float duration() const { return duration_; }
    float current_frame() const { return current_frame_; }
    void seek(float frame) { current_frame_ = frame; }

private:
    Animation anim_;
    float current_frame_ = 0.0f;
    float duration_ = 0.0f;
    std::unordered_map<std::string, int> name_to_idx_;
};
