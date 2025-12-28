import os
import sys

def escape_string(s):
    return 'R"(' + s + ')"'

# 把 shaders 集成成一个头文件
def main():
    shader_dir = "shaders"
    output_file = "include/EmbeddedShaders.h"
    
    if not os.path.exists(shader_dir):
        print(f"Error: {shader_dir} does not exist")
        return

    with open(output_file, "w", encoding="utf-8") as out:
        out.write("#pragma once\n")
        out.write("#include <string>\n")
        out.write("#include <unordered_map>\n\n")
        out.write("std::string get_embedded_shader(const std::string& filename) {\n")
        out.write("    static std::unordered_map<std::string, std::string> shaders = {\n")

        for root, dirs, files in os.walk(shader_dir):
            for file in files:
                if file.endswith(".glsl"):
                    path = os.path.join(root, file).replace("\\", "/")
                    with open(path, "r", encoding="utf-8") as f:
                        content = f.read()
                        out.write(f'        {{"{path}", {escape_string(content)}}},\n')

        out.write("    };\n")
        out.write("    auto it = shaders.find(filename);\n")
        out.write("    if (it != shaders.end()) return it->second;\n")
        out.write("    return \"\";\n")
        out.write("}\n")

if __name__ == "__main__":
    main()
