#include "VMDAnimation.h"
#include "TriMesh.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// Helper to convert Shift-JIS to UTF-8
std::string shift_jis_to_utf8(const std::string& sjis) {
#ifdef _WIN32
    int w_len = MultiByteToWideChar(932, 0, sjis.c_str(), -1, NULL, 0); // 932 is Shift-JIS
    if (w_len == 0) return sjis;
    std::vector<wchar_t> w_str(w_len);
    MultiByteToWideChar(932, 0, sjis.c_str(), -1, w_str.data(), w_len);
    
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, w_str.data(), -1, NULL, 0, NULL, NULL);
    if (utf8_len == 0) return sjis;
    std::vector<char> buffer(utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, w_str.data(), -1, buffer.data(), utf8_len, NULL, NULL);
    return std::string(buffer.data());
#else
    return sjis; // Non-Windows fallback (might need iconv)
#endif
}

VMDAnimation::VMDAnimation() {}
VMDAnimation::~VMDAnimation() {}

bool VMDAnimation::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    char header[30];
    file.read(header, 30);
    if (strncmp(header, "Vocaloid Motion Data 0002", 25) != 0) {
        std::cerr << "Invalid VMD header" << std::endl;
        return false;
    }

    char modelName[20];
    file.read(modelName, 20);

    int numBoneKeyframes;
    file.read(reinterpret_cast<char*>(&numBoneKeyframes), 4);

    for (int i = 0; i < numBoneKeyframes; ++i) {
        char boneName[15];
        file.read(boneName, 15);
        
        int frame;
        file.read(reinterpret_cast<char*>(&frame), 4);
        
        glm::vec3 pos;
        file.read(reinterpret_cast<char*>(&pos), 12);
        pos.z = -pos.z; // Coordinate fix
        
        float qx, qy, qz, qw;
        file.read(reinterpret_cast<char*>(&qx), 4);
        file.read(reinterpret_cast<char*>(&qy), 4);
        file.read(reinterpret_cast<char*>(&qz), 4);
        file.read(reinterpret_cast<char*>(&qw), 4);
        
        BoneKeyframe kf;
        kf.frame = frame;
        kf.position = pos;
        kf.rotation = glm::quat(qw, -qx, -qy, qz); 
        
        char interpolation[64];
        file.read(interpolation, 64);
        
        // Clean bone name (null terminated)
        std::string nameStr(boneName);
        size_t nullPos = nameStr.find('\0');
        if (nullPos != std::string::npos) nameStr = nameStr.substr(0, nullPos);
        
        // Convert to UTF-8
        nameStr = shift_jis_to_utf8(nameStr);

        tracks[nameStr].keyframes.push_back(kf);
        
        if ((float)frame > duration) duration = (float)frame;
    }
    
    // Sort keyframes by frame number
    for (auto& track : tracks) {
        std::sort(track.second.keyframes.begin(), track.second.keyframes.end(), 
            [](const BoneKeyframe& a, const BoneKeyframe& b) {
                return a.frame < b.frame;
            });
    }
    
    return true;
}

void VMDAnimation::update(float frame, TriMesh* mesh) {
    auto& bones = mesh->get_bones();
    const auto& mapping = mesh->get_bone_mapping();

    for (auto& trackPair : tracks) {
        std::string boneName = trackPair.first;
        
        // Try exact match
        if (mapping.find(boneName) == mapping.end()) {
            // Try converting Half-width IK to Full-width ＩＫ
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
        
        for (size_t i = 0; i < keyframes.size() - 1; ++i) {
            if (frame >= keyframes[i].frame && frame < keyframes[i+1].frame) {
                const auto& k1 = keyframes[i];
                const auto& k2 = keyframes[i+1];
                
                float t = (frame - k1.frame) / (float)(k2.frame - k1.frame);
                
                bone.local_translation = glm::mix(k1.position, k2.position, t);
                bone.local_rotation = glm::slerp(k1.rotation, k2.rotation, t);
                break;
            }
        }
    }
    
    mesh->update_bone_matrices();
}
