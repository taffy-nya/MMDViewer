#include "animation/anim_player.h"
#include "animation/skeleton.h"
#include "animation/morph.h"
#include "core/model.h"
#include <glm/gtc/quaternion.hpp>
#include <ranges>
#include <map>

auto AnimPlayer::set_anim(const Animation& anim,
                          const std::vector<BoneDef>& defs,
                          const std::vector<MorphDef>& morph_defs) -> std::expected<void, std::string> {
    anim_ = anim;

    name_to_idx_.clear();
    std::map<std::string, int> def_map;
    for (auto&& [i, def] : std::views::enumerate(defs)) {
        def_map[def.name] = static_cast<int>(i);
    }

    for (const auto& track : anim.tracks) {
        auto name = track.bone_name;
        auto it = def_map.find(name);
        if (it == def_map.end()) {
            size_t pos = name.find("IK");
            if (pos != std::string::npos) {
                std::string fw = name;
                fw.replace(pos, 2, "ＩＫ");
                it = def_map.find(fw);
            }
        }
        if (it != def_map.end()) {
            name_to_idx_[name] = it->second;
        }
    }

    morph_name_to_idx_.clear();
    std::map<std::string, int> morph_map;
    for (auto&& [i, def] : std::views::enumerate(morph_defs)) {
        morph_map[def.name] = static_cast<int>(i);
    }
    for (const auto& track : anim.morph_tracks) {
        auto it = morph_map.find(track.morph_name);
        if (it != morph_map.end()) {
            morph_name_to_idx_[track.morph_name] = it->second;
        }
    }

    duration_ = 0.0f;
    for (const auto& track : anim.tracks) {
        for (const auto& kf : track.keyframes) {
            if (static_cast<float>(kf.frame) > duration_) {
                duration_ = static_cast<float>(kf.frame);
            }
        }
    }
    for (const auto& track : anim.morph_tracks) {
        for (const auto& kf : track.keyframes) {
            if (static_cast<float>(kf.frame) > duration_) {
                duration_ = static_cast<float>(kf.frame);
            }
        }
    }

    current_frame_ = 0.0f;
    return {};
}

void AnimPlayer::update(float frame, const std::vector<BoneDef>& /*defs*/, Skeleton& skeleton) {
    if (anim_.tracks.empty()) return;

    auto& states = skeleton.states();

    for (const auto& track : anim_.tracks) {
        auto it = name_to_idx_.find(track.bone_name);
        if (it == name_to_idx_.end()) continue;

        int bone_idx = it->second;
        auto& st = states[bone_idx];
        const auto& kfs = track.keyframes;
        if (kfs.empty()) continue;

        if (frame <= static_cast<float>(kfs.front().frame)) {
            st.local_translation = kfs.front().translation;
            st.local_rotation    = kfs.front().rotation;
            continue;
        }
        if (frame >= static_cast<float>(kfs.back().frame)) {
            st.local_translation = kfs.back().translation;
            st.local_rotation    = kfs.back().rotation;
            continue;
        }

        for (size_t i = 0; i < kfs.size() - 1; ++i) {
            if (frame >= static_cast<float>(kfs[i].frame) &&
                frame < static_cast<float>(kfs[i + 1].frame)) {
                float t = (frame - kfs[i].frame) /
                          static_cast<float>(kfs[i + 1].frame - kfs[i].frame);
                st.local_translation = glm::mix(kfs[i].translation, kfs[i + 1].translation, t);
                st.local_rotation    = glm::slerp(kfs[i].rotation, kfs[i + 1].rotation, t);
                break;
            }
        }
    }
}

void AnimPlayer::update_morphs(float frame, MorphController& morph_ctrl) {
    if (anim_.morph_tracks.empty()) return;

    for (const auto& track : anim_.morph_tracks) {
        auto it = morph_name_to_idx_.find(track.morph_name);
        if (it == morph_name_to_idx_.end()) continue;

        int idx = it->second;
        const auto& kfs = track.keyframes;
        if (kfs.empty()) continue;

        if (frame <= static_cast<float>(kfs.front().frame)) {
            morph_ctrl.set_target_weight(idx, kfs.front().weight);
            continue;
        }
        if (frame >= static_cast<float>(kfs.back().frame)) {
            morph_ctrl.set_target_weight(idx, kfs.back().weight);
            continue;
        }

        for (size_t i = 0; i < kfs.size() - 1; ++i) {
            if (frame >= static_cast<float>(kfs[i].frame) &&
                frame < static_cast<float>(kfs[i + 1].frame)) {
                float t = (frame - kfs[i].frame) /
                          static_cast<float>(kfs[i + 1].frame - kfs[i].frame);
                float w = kfs[i].weight + (kfs[i + 1].weight - kfs[i].weight) * t;
                morph_ctrl.set_target_weight(idx, w);
                break;
            }
        }
    }
}
