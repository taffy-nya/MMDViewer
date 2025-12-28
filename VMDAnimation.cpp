#include "VMDAnimation.h"
#include "TriMesh.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>

#include <windows.h>

// 将 Shift-JIS 编码转换为 UTF-8
// VMD 文件中的字符串 (骨骼名称之类的) 一般是 Shift-JIS 编码
std::string shift_jis_to_utf8(const std::string& sjis) {
    int w_len = MultiByteToWideChar(932, 0, sjis.c_str(), -1, NULL, 0);
    if (w_len == 0) return sjis;
    std::vector<wchar_t> w_str(w_len);
    MultiByteToWideChar(932, 0, sjis.c_str(), -1, w_str.data(), w_len);
    
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, w_str.data(), -1, NULL, 0, NULL, NULL);
    if (utf8_len == 0) return sjis;
    std::vector<char> buffer(utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, w_str.data(), -1, buffer.data(), utf8_len, NULL, NULL);
    return std::string(buffer.data());
}

VMDAnimation::VMDAnimation() {}
VMDAnimation::~VMDAnimation() {}

bool VMDAnimation::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    // 读取文件头
    char header[30];
    file.read(header, 30);
    if (strncmp(header, "Vocaloid Motion Data 0002", 25) != 0) {
        // 只支持 v2
        std::cerr << "Invalid VMD header" << std::endl;
        return false;
    }

    char modelName[20];
    file.read(modelName, 20);

    // 骨骼关键帧数量
    int numBoneKeyframes;
    file.read(reinterpret_cast<char*>(&numBoneKeyframes), 4);

    for (int i = 0; i < numBoneKeyframes; ++i) {
        char boneName[15];
        file.read(boneName, 15);
        
        int frame;
        file.read(reinterpret_cast<char*>(&frame), 4);
        
        glm::vec3 pos;
        file.read(reinterpret_cast<char*>(&pos), 12);
        pos.z = -pos.z; // 跟 TriMesh 里一样的坑
        
        float qx, qy, qz, qw;
        file.read(reinterpret_cast<char*>(&qx), 4);
        file.read(reinterpret_cast<char*>(&qy), 4);
        file.read(reinterpret_cast<char*>(&qz), 4);
        file.read(reinterpret_cast<char*>(&qw), 4);
        
        BoneKeyframe kf;
        kf.frame = frame;
        kf.position = pos;
        kf.rotation = glm::quat(qw, -qx, -qy, qz); 
        
        char interpolation[64]; // 贝塞尔曲线插值参数 (跳过)
        file.read(interpolation, 64);
        
        // 清理骨骼名称 (确保以 null 结尾)
        std::string nameStr(boneName);
        size_t nullPos = nameStr.find('\0');
        if (nullPos != std::string::npos) nameStr = nameStr.substr(0, nullPos);
        
        // 转换为 UTF-8
        nameStr = shift_jis_to_utf8(nameStr);

        tracks[nameStr].keyframes.push_back(kf);
        
        if ((float)frame > duration) duration = (float)frame;
    }
    
    // 按帧号排序关键帧
    for (auto& track : tracks) {
        std::sort(track.second.keyframes.begin(), track.second.keyframes.end(), 
            [](const BoneKeyframe& a, const BoneKeyframe& b) {
                return a.frame < b.frame;
            });
    }
    
    return true;
}

// 更新模型姿态
// frame 为当前播放的帧数, 这里 float 可以是小数, 用于插值
void VMDAnimation::update(float frame, TriMesh *mesh) {
    auto& bones = mesh->get_bones();
    const auto& mapping = mesh->get_bone_mapping();

    for (auto& trackPair : tracks) {
        std::string boneName = trackPair.first;
        
        // 尝试精确匹配骨骼名称
        if (mapping.find(boneName) == mapping.end()) {
            // 传奇编码问题之半角 IK 与全角ＩＫ
            size_t pos = boneName.find("IK");
            if (pos != std::string::npos) {
                std::string fullWidthName = boneName;
                fullWidthName.replace(pos, 2, "ＩＫ");
                if (mapping.find(fullWidthName) != mapping.end()) {
                    boneName = fullWidthName;
                }
            }
        }
        
        if (mapping.find(boneName) == mapping.end()) continue;
        
        int boneIndex = mapping.at(boneName);
        PMXBone& bone = bones[boneIndex];
        const auto& keyframes = trackPair.second.keyframes;
        
        if (keyframes.empty()) continue;
        
        // 边界情况处理
        if (frame <= keyframes.front().frame) {
            bone.local_translation = keyframes.front().position;
            bone.local_rotation = keyframes.front().rotation;
            continue;
        }
        if (frame >= keyframes.back().frame) {
            bone.local_translation = keyframes.back().position;
            bone.local_rotation = keyframes.back().rotation;
            continue;
        }
        
        // 寻找当前帧的前后关键帧进行插值 (应该可以优化)
        for (size_t i = 0; i < keyframes.size() - 1; ++i) {
            if (frame >= keyframes[i].frame && frame < keyframes[i+1].frame) {
                const auto& k1 = keyframes[i];
                const auto& k2 = keyframes[i+1];
                
                // 插值系数 t in [0, 1]
                float t = (frame - k1.frame) / (float)(k2.frame - k1.frame);
                
                // 线性插值
                bone.local_translation = glm::mix(k1.position, k2.position, t);
                bone.local_rotation = glm::slerp(k1.rotation, k2.rotation, t);
                break;
            }
        }
    }
    
    // 更新所有骨骼的全局变换矩阵
    mesh->update_bone_matrices();
}
