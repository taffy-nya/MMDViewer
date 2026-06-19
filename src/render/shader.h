#pragma once
#include <glad/glad.h>
#include <expected>
#include <string>
#include <string_view>
#include <unordered_map>
#include <glm/glm.hpp>

namespace render {

class Shader {
public:
    static auto load(std::string_view vert_key, std::string_view frag_key)
        -> std::expected<Shader, std::string>;

    static auto from_paths(const char* vert_path, const char* frag_path)
        -> std::expected<Shader, std::string>;

    Shader() = default;
    ~Shader();
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const { glUseProgram(program_); }
    GLuint id() const { return program_; }
    explicit operator bool() const { return program_ != 0; }

    auto uniform(const char* name) const -> GLint;

    void set_mat4(const char* name, const glm::mat4& m) const;
    void set_vec3(const char* name, const glm::vec3& v) const;
    void set_vec4(const char* name, const glm::vec4& v) const;
    void set_float(const char* name, float v) const;
    void set_int(const char* name, int v) const;

private:
    explicit Shader(GLuint p) : program_(p) {}
    GLuint program_ = 0;
    mutable std::unordered_map<std::string, GLint> cache_;
};

} // namespace render
