#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"
#include "VMDAnimation.h"
#include "Stage.h"
#include "Light.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include <vector>
#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>

// 将 Windows 路径的 ANSI 编码转换为 UTF-8 编码
std::string AnsiToUtf8(const std::string& str) {
    if (str.empty()) return "";
    int wlen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    if (wlen <= 0) return str;
    std::vector<wchar_t> wstr(wlen);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wstr.data(), wlen);
    
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, NULL, 0, NULL, NULL);
    if (len <= 0) return str;
    std::vector<char> utf8(len);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, utf8.data(), len, NULL, NULL);
    
    return std::string(utf8.data());
}

std::string open_file_dialog(const char* filter, GLFWwindow* window) {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(window);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = "";
    
    if (GetOpenFileNameA(&ofn))
        return std::string(fileName);
    return "";
}

// 一些全局变量
char pmx_path_buf[256] = "models/taffy/taffy.pmx";
char vmd_path_buf[256] = "motions/TDA.vmd";
char stage_path_buf[256] = "Default Grid";

Camera* camera = nullptr;
MeshPainter* painter = nullptr;
TriMesh* mesh = nullptr;
VMDAnimation* animation = nullptr;
Stage* stage = nullptr;

bool mouse_left_pressed = false, mouse_right_pressed = false, mouse_middle_pressed = false;
double rotate_sensitivity = 0.2, translate_sensitivity = 0.05;
double last_x, last_y;

// 光照
std::vector<Light> lights;
glm::vec3 ambient_color(1.0f, 1.0f, 1.0f);
float ambient_strength = 1.0f;
glm::vec4 clear_color = glm::vec4(0.5f, 0.6f, 0.7f, 1.0f);

// 阴影映射
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// 动画
float last_frame_time = 0.0f;
float current_frame = 0.0f;
bool is_playing = true;
bool show_stage = true;
bool enable_motion = false;

// UI 控制
bool show_light_gizmos = true;
bool show_skeleton = false;
bool manual_bone_control = false;
int selected_bone_index = -1;
int selected_light_index = -1;
bool show_control_window = true;
ImGuizmo::OPERATION current_gizmo_operation = ImGuizmo::ROTATE;
ImGuizmo::MODE current_gizmo_mode = ImGuizmo::LOCAL;

// FPS 限制
int target_fps = 60;
bool limit_fps = true;

// 函数声明
void framebuffer_size_callback(GLFWwindow *w, int width, int height);
void key_callback(GLFWwindow* w, int k, int s, int a, int m);
void mouse_button_callback(GLFWwindow* w, int b, int a, int m);
void cursor_position_callback(GLFWwindow* w, double x, double y);
void scroll_callback(GLFWwindow* w, double x, double y);
void init();
void display();
void clean_up();
void reset();
void print_help();
void load_model(const std::string& path);
void load_motion(const std::string& path);

// 初始化
void init() {
    camera = new Camera();
    painter = new MeshPainter();
    stage = new Stage(100.0f, 20);
    
    // 初始添加一个平行光   // 初始不添加一个平行光
    // if (lights.empty()) {
    //     Light mainLight;
    //     mainLight.type = LIGHT_DIRECTIONAL;
    //     mainLight.direction = glm::vec3(-0.5f, -1.0f, -0.5f);
    //     mainLight.color = glm::vec3(1.0f);
    //     mainLight.intensity = 0.5f;
    //     lights.push_back(mainLight);
    // }

    load_model(pmx_path_buf);
    load_motion(vmd_path_buf);

    // 配置阴影映射 FBO (Frame Buffer Object)
    // 从光源的视角渲染场景，记录深度值到纹理中
    glGenFramebuffers(1, &depthMapFBO);

    // 创建深度纹理
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    // 设置纹理大小为 SHADOW_WIDTH * SHADOW_HEIGHT
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 设置纹理比较模式, 用于在着色器中进行阴影测试
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    // 设置纹理环绕方式, 超出范围的部分设为白色 (深度 1.0), 避免阴影之外的区域产生错误阴影
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    // 这里只关心深度, 不渲染颜色数据
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST); // 开启深度测试
    glEnable(GL_BLEND);      // 开启混合 (用于透明材质)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void load_model(const std::string& path) {
    if (mesh) {
        delete mesh;
        mesh = nullptr;
    }
    if (painter) {
        delete painter;
        painter = new MeshPainter();
    }

    mesh = new TriMesh();
    mesh->read_pmx(path);
    
    std::string base_path = "";
    size_t last_slash = path.rfind('/');
    if(last_slash == std::string::npos) last_slash = path.rfind('\\');
    if(last_slash != std::string::npos) base_path = path.substr(0, last_slash + 1);
    mesh->load_opengl_textures(base_path);

    painter->add_mesh(mesh);
}

void load_motion(const std::string& path) {
    if (animation) {
        delete animation;
    }
    animation = new VMDAnimation();
    if (!animation->load(path)) {
        std::cerr << "Failed to load VMD: " << path << std::endl;
    } else {
        std::cout << "Loaded VMD: " << path << " (" << animation->get_duration() << " frames)" << std::endl;
    }
    current_frame = 0.0f;
}


void display() {
    // 阴影
    // 从光源的视角渲染场景，生成深度贴图
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 100.0f;
    
    // 使用第一个光源作为产生阴影的光源
    glm::vec3 lightPos = glm::vec3(-2.0f, 4.0f, -1.0f); // 默认位置
    if (!lights.empty()) {
        if (lights[0].type == LIGHT_DIRECTIONAL) {
            // 平行光使用阴影正交投影矩阵
            lightPos = -lights[0].direction * 20.0f;
            lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
        } else {
            // 点光源使用透视投影
            lightPos = lights[0].position;
            lightProjection = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        }
    } else {
         lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
    }
    
    lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView; // 光源空间的变换矩阵
    
    // 正面剔除对于双面材质可能会有问题，这里注释掉
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);
    
    if (show_stage && stage) stage->draw_shadow(lightSpaceMatrix);
    painter->draw_shadow(lightSpaceMatrix);
    
    // glCullFace(GL_BACK); // 重置剔除设置
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 光照
    // 恢复正常的视口和帧缓冲，进行正常的场景渲染
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 绘制场景和模型，传入阴影贴图和光空间矩阵用于计算阴影
    if (show_stage && stage) stage->draw(camera, lights, ambient_color, ambient_strength, depthMap, lightSpaceMatrix);
    painter->draw_meshes(camera, lights, ambient_color, ambient_strength, depthMap, lightSpaceMatrix);
    
    // 绘制辅助线框
    if (show_light_gizmos) painter->draw_light_gizmos(camera, lights);
    if (show_skeleton) painter->draw_skeleton(camera, selected_bone_index);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { 
    if (height == 0) height = 1;
    glViewport(0, 0, width, height); 
    if (camera) {
        camera->aspect = (float)width / (float)height;
        // camera->update_camera_vectors();
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // 根据两帧之间鼠标位置的变化移动
    double deltaX = xpos - last_x;
    double deltaY = ypos - last_y;
    
    if (mouse_left_pressed) { 
        if (camera) camera->orbit(deltaX * rotate_sensitivity * 2.0f, deltaY * rotate_sensitivity * 2.0f);
    }
    if (mouse_right_pressed) { 
        if (camera) camera->pan(deltaX * translate_sensitivity, deltaY * translate_sensitivity);
    }
    last_x = xpos; last_y = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) return;
        glfwGetCursorPos(window, &last_x, &last_y);
        if (button == GLFW_MOUSE_BUTTON_LEFT) mouse_left_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) mouse_right_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) mouse_middle_pressed = true;
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) mouse_left_pressed = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) mouse_right_pressed = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) mouse_middle_pressed = false;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) return;
    if (camera) {
        camera->handle_scroll(yoffset);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (camera) camera->handle_keys(key, action);
        
        switch (key) {
            case GLFW_KEY_R: reset(); break;
            case GLFW_KEY_C: show_control_window = !show_control_window; break;
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
            default: break;
        }
    }
}

void print_help() {
    printf("--- MMD Model Viewer Controls ---\n");
    printf("Mouse:\n");
    printf("  Left Drag:   Rotate camera\n");
    printf("  Right Drag:  Pan camera\n");
    printf("  Scroll:      Zoom in/out\n\n");
    printf("Keyboard:\n");
    printf("  W, A, S, D:  Move camera\n");
    printf("  R:           Reset all transformations\n");
    printf("  C:           Toggle control window\n");
    printf("  ESC:         Exit\n");
    printf("---------------------------------\n");
}

void clean_up() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (painter) delete painter;
    if (mesh) delete mesh;
    if (camera) delete camera;
    if (animation) delete animation;
    if (stage) delete stage;
}

void reset() {
    if (mesh) {
        mesh->set_scale(glm::vec3(1.0f));
        mesh->set_rotation(glm::vec3(0.0f));
        mesh->set_translation(glm::vec3(0.0f));
    }
    if (camera) camera->reset();
}

int main(int argc, char** argv) {
    timeBeginPeriod(1); // 提高定时器精度，不然 FPS 限制效果不太好

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "MMD Model Viewer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // 禁用垂直同步以允许手动限制 FPS

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    init();

    // 由于窗口不是方的，会被拉伸，这里手动调整视口比例
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    print_help();

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 设置中文字体
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    // ImGui 样式
    ImGui::StyleColorsDark();

    // 设置 ImGui 的平台与渲染器后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);

    last_frame_time = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - last_frame_time;
        last_frame_time = currentTime;

        // 动画更新逻辑
        if (enable_motion && !manual_bone_control) {
            if (is_playing && animation) {
                // VMD 标准帧率为 30 FPS
                current_frame += deltaTime * 30.0f; 
                if (current_frame >= animation->get_duration()) {
                    current_frame = 0.0f;
                }
                // 根据当前帧更新骨骼姿态
                animation->update(current_frame, mesh);
            }
        } else if (!enable_motion && !manual_bone_control) {
            // 不启用动画且非手动控制时，重置模型为 T-Pose
            if (mesh) {
                mesh->reset_pose();
            }
        }
        
        // 这里需要更新骨骼矩阵
        // 因为 T-Pose 或手动控制，骨骼矩阵也需要重新计算并传递给着色器
        if (mesh) {
            mesh->update_bone_matrices();
        }

        // ImGui 控制界面
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        if (show_control_window) {
            ImGui::Begin("MMD Viewer Controls");
            
            if (ImGui::BeginTabBar("ControlTabs")) {
                // General 标签页，显示 FPS、相机重置、背景色等
                if (ImGui::BeginTabItem("General")) {
                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                    ImGui::Separator();
                    ImGui::Text("Performance");
                    ImGui::Checkbox("Limit FPS", &limit_fps);
                    if (limit_fps) {
                        ImGui::SliderInt("Target FPS", &target_fps, 10, 240);
                    }
                    ImGui::Separator();
                    ImGui::Text("Camera");
                    if (ImGui::Button("Reset Camera")) {
                        if (camera) camera->reset();
                    }
                    ImGui::Separator();
                    ImGui::Text("Background");
                    ImGui::ColorEdit3("Clear Color", (float*)&clear_color);
                    ImGui::EndTabItem();
                }

                // Model 标签页，加载模型与动作，调整模型变换与动画播放
                if (ImGui::BeginTabItem("Model")) {
                    ImGui::Text("Current Model: %s", AnsiToUtf8(pmx_path_buf).c_str());
                    if (ImGui::Button("Load Model (.pmx)")) {
                        std::string path = open_file_dialog("PMX Files (*.pmx)\0*.pmx\0All Files (*.*)\0*.*\0", window);
                        if (!path.empty()) {
                            strncpy(pmx_path_buf, path.c_str(), sizeof(pmx_path_buf) - 1);
                            load_model(pmx_path_buf);
                        }
                    }
                    ImGui::Text("Current Motion: %s", AnsiToUtf8(vmd_path_buf).c_str());
                    if (ImGui::Button("Load Motion (.vmd)")) {
                        std::string path = open_file_dialog("VMD Files (*.vmd)\0*.vmd\0All Files (*.*)\0*.*\0", window);
                        if (!path.empty()) {
                            strncpy(vmd_path_buf, path.c_str(), sizeof(vmd_path_buf) - 1);
                            load_motion(vmd_path_buf);
                        }
                    }
                    
                    ImGui::Separator();
                    ImGui::Text("Transform");
                    if (mesh) {
                        glm::vec3 pos = mesh->get_translation();
                        if (ImGui::DragFloat3("##Pos", (float*)&pos, 0.1f)) {
                            mesh->set_translation(pos);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##Pos")) {
                            mesh->set_translation(glm::vec3(0.0f));
                        }
                        ImGui::SameLine();
                        ImGui::Text("Position");

                        glm::vec3 rot = mesh->get_rotation();
                        if (ImGui::DragFloat3("##Rot", (float*)&rot, 1.0f)) {
                            mesh->set_rotation(rot);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##Rot")) {
                            mesh->set_rotation(glm::vec3(0.0f));
                        }
                        ImGui::SameLine();
                        ImGui::Text("Rotation");

                        glm::vec3 scale = mesh->get_scale();
                        if (ImGui::DragFloat3("##Scale", (float*)&scale, 0.01f)) {
                            mesh->set_scale(scale);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##Scale")) {
                            mesh->set_scale(glm::vec3(1.0f));
                        }
                        ImGui::SameLine();
                        ImGui::Text("Scale");
                    }
                    // 动画控制
                    ImGui::Separator();
                    ImGui::Text("Animation");
                    ImGui::Checkbox("Enable Motion", &enable_motion);
                    if (enable_motion) {
                        ImGui::Checkbox("Play Animation", &is_playing);
                    }
                    ImGui::EndTabItem();
                }

                // Skeleton 标签页，显示骨骼列表与手动控制选项
                if (ImGui::BeginTabItem("Skeleton")) {
                    ImGui::Checkbox("Show Skeleton", &show_skeleton);
                    ImGui::Checkbox("Manual Control", &manual_bone_control);
                    if (manual_bone_control) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "(Animation Paused)");
                    }

                    ImGui::Separator();
                    if (mesh) {
                        if (ImGui::Button("Reset All Bones")) {
                            mesh->reset_pose();
                            selected_bone_index = -1;
                        }
                    }
                    
                    
                    if (mesh) {
                        auto& bones = mesh->get_bones();
                        
                        // 左边显示骨骼列表
                        ImGui::BeginChild("BoneList", ImVec2(150, 0), true);
                        for (int i = 0; i < bones.size(); ++i) {
                            std::string label = std::to_string(i) + ": " + bones[i].name;
                            if (ImGui::Selectable(label.c_str(), selected_bone_index == i)) {
                                selected_bone_index = i;
                            }
                        }
                        ImGui::EndChild();
                        
                        ImGui::SameLine();
                        
                        // 右边显示骨骼详情
                        ImGui::BeginGroup();
                        if (selected_bone_index >= 0 && selected_bone_index < bones.size()) {
                            PMXBone& bone = bones[selected_bone_index];
                            ImGui::Text("Bone: %s", bone.name.c_str());
                            ImGui::Text("ID: %d", selected_bone_index);
                            ImGui::Text("Parent: %d", bone.parent_index);
                            
                            ImGui::Separator();
                            
                            if (manual_bone_control) {
                                // 选择变换操作（通过 ImGuizmo 控件）
                                if (ImGui::RadioButton("Rotate", current_gizmo_operation == ImGuizmo::ROTATE)) current_gizmo_operation = ImGuizmo::ROTATE;
                                ImGui::SameLine();
                                if (ImGui::RadioButton("Translate", current_gizmo_operation == ImGuizmo::TRANSLATE)) current_gizmo_operation = ImGuizmo::TRANSLATE;
                                
                                // 平移
                                ImGui::Text("Local Translation");
                                if (ImGui::DragFloat3("##BoneTrans", (float*)&bone.local_translation, 0.01f)) {
                                    // 直接修改 local_translation 即可
                                }
                                if (ImGui::Button("Reset Trans")) {
                                    bone.local_translation = glm::vec3(0.0f);
                                }
                                
                                // 旋转
                                ImGui::Text("Local Rotation");
                                glm::vec3 euler = glm::degrees(glm::eulerAngles(bone.local_rotation));
                                if (ImGui::DragFloat3("##BoneRot", (float*)&euler, 1.0f)) {
                                    bone.local_rotation = glm::quat(glm::radians(euler));
                                }
                                if (ImGui::Button("Reset Rot")) {
                                    bone.local_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                                }

                                // ImGuizmo 返回的矩阵变换在后面处理
                            } else {
                                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Enable Manual Control to edit");
                                ImGui::Text("Translation: %.2f, %.2f, %.2f", bone.local_translation.x, bone.local_translation.y, bone.local_translation.z);
                                glm::vec3 euler = glm::degrees(glm::eulerAngles(bone.local_rotation));
                                ImGui::Text("Rotation: %.2f, %.2f, %.2f", euler.x, euler.y, euler.z);
                            }
                        } else {
                            ImGui::Text("Select a bone to edit");
                        }
                        ImGui::EndGroup();
                    } else {
                        ImGui::Text("No model loaded");
                    }
                    
                    ImGui::EndTabItem();
                }
                // Stage 标签页，加载与显示舞台
                if (ImGui::BeginTabItem("Stage")) {
                    ImGui::Checkbox("Show Stage", &show_stage);
                    ImGui::Text("Current Stage: %s", AnsiToUtf8(stage_path_buf).c_str());
                    if (ImGui::Button("Load Stage PMX")) {
                        std::string path = open_file_dialog("PMX Files (*.pmx)\0*.pmx\0All Files (*.*)\0*.*\0", window);
                        if (!path.empty()) {
                            strncpy(stage_path_buf, path.c_str(), sizeof(stage_path_buf) - 1);
                            stage->load_pmx(path);
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Stage")) {
                        strcpy(stage_path_buf, "Default Grid");
                        stage->use_default_grid();
                    }
                    ImGui::EndTabItem();
                }
                // Lighting 标签页，管理光源与环境光
                if (ImGui::BeginTabItem("Lighting")) {
                    ImGui::Checkbox("Show Light Gizmos", &show_light_gizmos);
                    ImGui::Text("Ambient Light");
                    ImGui::ColorEdit3("Color", (float*)&ambient_color);
                    ImGui::SliderFloat("Strength", &ambient_strength, 0.0f, 1.0f);

                    ImGui::Separator();
                    ImGui::Text("Dynamic Lights");
                    if (ImGui::Button("Add Light")) {
                        if (lights.size() < 16) {
                            Light l;
                            l.type = lights.empty() ? LIGHT_DIRECTIONAL : LIGHT_POINT;
                            l.position = glm::vec3(0.0f, 10.0f, 0.0f);
                            lights.push_back(l);
                        }
                    }
                    
                    ImGui::Separator();
                    
                    if (selected_light_index != -1) {
                        if (ImGui::Button("Deselect Light")) {
                            selected_light_index = -1;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset Selected Light")) {
                            if (selected_light_index >= 0 && selected_light_index < (int)lights.size()) {
                                Light &l = lights[selected_light_index];
                                l.color = glm::vec3(1.0f);
                                l.intensity = 1.0f;
                                l.enabled = true;
                                l.constant = 1.0f;
                                l.linear = 0.09f;
                                l.quadratic = 0.032f;
                                if (l.type == LIGHT_POINT) {
                                    l.position = glm::vec3(0.0f, 10.0f, 0.0f);
                                } else {
                                    l.direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
                                }
                            }
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset All Lights")) {
                        for (auto &l : lights) {
                            l.color = glm::vec3(1.0f);
                            l.intensity = 1.0f;
                            l.enabled = true;
                            l.constant = 1.0f;
                            l.linear = 0.09f;
                            l.quadratic = 0.032f;
                            if (l.type == LIGHT_POINT) {
                                l.position = glm::vec3(0.0f, 10.0f, 0.0f);
                            } else {
                                l.direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
                            }
                        }
                    }

                    for (int i = 0; i < lights.size(); ++i) {
                        ImGui::PushID(i);
                        // 高亮选中的光源
                        bool isSelected = (selected_light_index == i);
                        ImGuiTreeNodeFlags flags = isSelected ? ImGuiTreeNodeFlags_Selected : 0;
                        bool isOpen = ImGui::TreeNodeEx(("Light " + std::to_string(i)).c_str(), flags);
                        
                        if (ImGui::IsItemClicked()) {
                            selected_light_index = i;
                            selected_bone_index = -1;
                        }

                        if (isOpen) {
                            ImGui::Checkbox("Enabled", &lights[i].enabled);
                            const char* types[] = { "Directional", "Point" };
                            ImGui::Combo("Type", &lights[i].type, types, IM_ARRAYSIZE(types));
                            
                            if (lights[i].type == LIGHT_DIRECTIONAL) {
                                // 更新方向光的方向
                                // Pitch = asin(-y)
                                // Yaw = atan2(x, z)
                                
                                float pitch = glm::degrees(asin(glm::clamp(-lights[i].direction.y, -1.0f, 1.0f)));
                                float yaw = glm::degrees(atan2(lights[i].direction.x, lights[i].direction.z));
                                
                                bool changed = false;
                                changed |= ImGui::DragFloat("Pitch", &pitch, 1.0f, -89.0f, 89.0f);
                                changed |= ImGui::DragFloat("Yaw", &yaw, 1.0f, -180.0f, 180.0f);
                                
                                if (changed) {
                                    float rPitch = glm::radians(pitch);
                                    float rYaw = glm::radians(yaw);
                                    lights[i].direction.x = sin(rYaw) * cos(rPitch);
                                    lights[i].direction.y = -sin(rPitch);
                                    lights[i].direction.z = cos(rYaw) * cos(rPitch);
                                    lights[i].direction = glm::normalize(lights[i].direction);
                                }
                                
                                ImGui::TextDisabled("Raw Dir: (%.2f, %.2f, %.2f)", lights[i].direction.x, lights[i].direction.y, lights[i].direction.z);

                            } else {
                                ImGui::DragFloat3("Position", (float*)&lights[i].position, 0.5f);
                                ImGui::DragFloat("Constant", &lights[i].constant, 0.01f, 0.0f, 10.0f);
                                ImGui::DragFloat("Linear", &lights[i].linear, 0.001f, 0.0f, 1.0f);
                                ImGui::DragFloat("Quadratic", &lights[i].quadratic, 0.0001f, 0.0f, 1.0f);
                            }
                            
                            ImGui::ColorEdit3("Color", (float*)&lights[i].color);
                            ImGui::SliderFloat("Intensity", &lights[i].intensity, 0.0f, 5.0f);
                            
                            if (ImGui::Button("Remove")) {
                                lights.erase(lights.begin() + i);
                                if (selected_light_index == i) selected_light_index = -1;
                                ImGui::TreePop();
                                ImGui::PopID();
                                i--;
                                continue; 
                            }
                            
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        // 通过 ImGuizmo 返回的矩阵变换来更新骨骼或灯光
        if (camera) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

            glm::mat4 view = camera->get_view_matrix();
            glm::mat4 projection = camera->get_projection_matrix();

            // 骨骼控制
            if (manual_bone_control && mesh && selected_bone_index >= 0) {
                // 如果选择了骨骼，取消选择灯光
                selected_light_index = -1;

                auto& bones = mesh->get_bones();
                if (selected_bone_index < bones.size()) {
                    PMXBone& bone = bones[selected_bone_index];
                    glm::mat4 modelMatrix = mesh->get_model_matrix();
                    glm::mat4 globalTransform = modelMatrix * bone.global_transform;

                    if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), 
                        current_gizmo_operation, current_gizmo_mode, glm::value_ptr(globalTransform))) {
                        
                        glm::mat4 parentGlobal = glm::mat4(1.0f);
                        if (bone.parent_index != -1) {
                            parentGlobal = bones[bone.parent_index].global_transform;
                        }
                        
                        glm::mat4 parentWorld = modelMatrix * parentGlobal;
                        glm::mat4 localTransform = glm::inverse(parentWorld) * globalTransform;
                        
                        glm::vec3 scale;
                        glm::quat rotation;
                        glm::vec3 translation;
                        glm::vec3 skew;
                        glm::vec4 perspective;
                        glm::decompose(localTransform, scale, rotation, translation, skew, perspective);
                        
                        // 计算相对位置以修正平移
                        glm::vec3 parentPos = glm::vec3(0.0f);
                        if (bone.parent_index != -1) {
                            parentPos = bones[bone.parent_index].position;
                        }
                        glm::vec3 relativePos = bone.position - parentPos;

                        // 大坑：ImGuizmo 的变换矩阵包含了骨骼相对于父骨骼的完整位移
                        // 但是这里 local_translation 仅存储偏移量，需要减去初始相对位置
                        bone.local_translation = translation - relativePos;
                        bone.local_rotation = rotation;
                    }
                }
            }
            
            // 灯光控制
            if (show_light_gizmos && selected_light_index >= 0 && selected_light_index < lights.size()) {
                Light& light = lights[selected_light_index];
                glm::mat4 transform = glm::mat4(1.0f);
                
                if (light.type == LIGHT_POINT) {
                    transform = glm::translate(transform, light.position);
                } else {
                    // 平行光
                    // gizmo 放在 (0, 10, 0)
                    glm::vec3 displayPos = glm::vec3(0.0f, 10.0f, 0.0f); 
                    transform = glm::translate(transform, displayPos);
                    
                    glm::vec3 defaultDir = glm::vec3(0.0f, -1.0f, 0.0f); // 向下
                    
                    // 计算从默认方向 (0, -1, 0) 到当前方向的旋转
                    glm::quat q = glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), glm::normalize(light.direction));
                    transform = transform * glm::toMat4(q);
                }

                ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
                if (light.type == LIGHT_DIRECTIONAL) op = ImGuizmo::ROTATE;
                
                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), 
                    op, ImGuizmo::WORLD, glm::value_ptr(transform))) {
                    
                    if (light.type == LIGHT_POINT) {
                        light.position = glm::vec3(transform[3]);
                    } else {
                        glm::mat3 rotMat = glm::mat3(transform);
                        light.direction = glm::normalize(rotMat * glm::vec3(0.0f, -1.0f, 0.0f));
                    }
                }
            }
        }

        ImGui::Render();

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        display();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        
        glfwPollEvents();

        // FPS 限制
        if (limit_fps && target_fps > 0) {
            float frameTime = (float)glfwGetTime() - currentTime;
            float targetFrameTime = 1.0f / (float)target_fps;
            if (frameTime < targetFrameTime) {
                float sleepTime = targetFrameTime - frameTime;
                if (sleepTime > 0.002f) { // 如果剩余时间超过 2ms 则休眠
                    Sleep((DWORD)(sleepTime * 1000.0f - 1.0f)); 
                }
                while ((float)glfwGetTime() - currentTime < targetFrameTime) {
                    // 等
                }
            }
        }
    }

    clean_up();
    glfwTerminate();
    timeEndPeriod(1);
    return 0;
}