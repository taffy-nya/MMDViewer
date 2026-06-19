#include "render/shader.h"
#include "shader_sources.h"
#include <print>
#include <glm/gtc/type_ptr.hpp>

namespace render {

static auto key_from_path(std::string_view path) -> std::string_view {
    auto start = path.find_last_of("/\\");
    start = (start == std::string_view::npos) ? 0 : start + 1;
    auto end = path.find_last_of('.');
    if (end == std::string_view::npos) end = path.size();
    return path.substr(start, end - start);
}

static auto compile_stage(GLenum type, std::string_view src) -> std::expected<GLuint, std::string> {
    auto code = src.data();
    auto len  = static_cast<GLint>(src.size());
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, &len);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<size_t>(log_len > 0 ? log_len - 1 : 0), '\0');
        glGetShaderInfoLog(shader, log_len, nullptr, log.data());
        glDeleteShader(shader);
        return std::unexpected(std::move(log));
    }
    return shader;
}

auto Shader::load(std::string_view vert_key, std::string_view frag_key) -> std::expected<Shader, std::string> {
    auto it = k_shader_sources.find(vert_key);
    if (it == k_shader_sources.end()) {
        return std::unexpected(std::string("embedded shader not found: ") + std::string(vert_key));
    }
    auto vert_src = it->second;

    it = k_shader_sources.find(frag_key);
    if (it == k_shader_sources.end()) {
        return std::unexpected(std::string("embedded shader not found: ") + std::string(frag_key));
    }
    auto frag_src = it->second;

    auto vs = compile_stage(GL_VERTEX_SHADER, vert_src);
    if (!vs) {
        std::println(stderr, "[shader] vert compile failed: {}", vs.error());
        return std::unexpected(std::move(vs.error()));
    }

    auto fs = compile_stage(GL_FRAGMENT_SHADER, frag_src);
    if (!fs) {
        std::println(stderr, "[shader] frag compile failed: {}", fs.error());
        glDeleteShader(*vs);
        return std::unexpected(std::move(fs.error()));
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, *vs);
    glAttachShader(program, *fs);
    glLinkProgram(program);

    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<size_t>(log_len > 0 ? log_len - 1 : 0), '\0');
        glGetProgramInfoLog(program, log_len, nullptr, log.data());
        std::println(stderr, "[shader] link failed: {}", log);
        glDeleteProgram(program);
        glDeleteShader(*vs);
        glDeleteShader(*fs);
        return std::unexpected(std::move(log));
    }

    glDeleteShader(*vs);
    glDeleteShader(*fs);
    return Shader(program);
}

auto Shader::from_paths(const char* vert_path, const char* frag_path) -> std::expected<Shader, std::string> {
    return load(key_from_path(vert_path), key_from_path(frag_path));
}

Shader::~Shader() {
    if (program_ != 0) glDeleteProgram(program_);
}

Shader::Shader(Shader&& other) noexcept : program_(other.program_), cache_(std::move(other.cache_)) {
    other.program_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (program_ != 0) glDeleteProgram(program_);
        program_ = other.program_;
        cache_ = std::move(other.cache_);
        other.program_ = 0;
    }
    return *this;
}

auto Shader::uniform(const char* name) const -> GLint {
    auto it = cache_.find(name);
    if (it != cache_.end()) return it->second;
    GLint loc = glGetUniformLocation(program_, name);
    cache_[name] = loc;
    return loc;
}

void Shader::set_mat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(uniform(name), 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::set_vec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(uniform(name), 1, glm::value_ptr(v));
}

void Shader::set_vec4(const char* name, const glm::vec4& v) const {
    glUniform4fv(uniform(name), 1, glm::value_ptr(v));
}

void Shader::set_float(const char* name, float v) const {
    glUniform1f(uniform(name), v);
}

void Shader::set_int(const char* name, int v) const {
    glUniform1i(uniform(name), v);
}

} // namespace render
