#include "anim_loader.h"
#include "platform/encoding.h"
#include <fstream>
#include <format>
#include <algorithm>
#include <cstring>

namespace core {

template <typename T>
static void read_data(std::ifstream& file, T& val) {
    file.read(reinterpret_cast<char*>(&val), sizeof(T));
}

auto load_vmd(const std::filesystem::path& path) -> std::expected<Animation, std::string> {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(std::format("cannot open VMD: {}", path.string()));
    }

    char header[30];
    file.read(header, 30);
    if (strncmp(header, "Vocaloid Motion Data 0002", 25) != 0) {
        return std::unexpected("invalid VMD header (only v2 supported)");
    }

    char model_name[20];
    file.read(model_name, 20);

    Animation anim;

    int num_keyframes;
    read_data(file, num_keyframes);

    for (int i = 0; i < num_keyframes; ++i) {
        char bone_name[15];
        file.read(bone_name, 15);

        BoneKeyframe kf;
        read_data(file, kf.frame);

        read_data(file, kf.translation);
        kf.translation.z = -kf.translation.z; // MMD → OpenGL

        float qx, qy, qz, qw;
        read_data(file, qx);
        read_data(file, qy);
        read_data(file, qz);
        read_data(file, qw);
        kf.rotation = glm::quat(qw, -qx, -qy, qz);

        file.read(reinterpret_cast<char*>(kf.bezier_params.data()), 64);

        std::string name_str(bone_name);
        size_t null_pos = name_str.find('\0');
        if (null_pos != std::string::npos) name_str = name_str.substr(0, null_pos);
        name_str = encoding::sjis_to_utf8(name_str);

        // Find or create track
        auto it = std::find_if(anim.tracks.begin(), anim.tracks.end(),
            [&](const BoneTrack& t) { return t.bone_name == name_str; });
        if (it == anim.tracks.end()) {
            anim.tracks.push_back({name_str, {}});
            it = anim.tracks.end() - 1;
        }
        it->keyframes.push_back(kf);
    }

    // Sort keyframes per track
    for (auto& track : anim.tracks) {
        std::sort(track.keyframes.begin(), track.keyframes.end(),
            [](const BoneKeyframe& a, const BoneKeyframe& b) { return a.frame < b.frame; });
    }

    return anim;
}

} // namespace core
