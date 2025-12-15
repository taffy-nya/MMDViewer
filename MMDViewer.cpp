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

#include <vector>
#include <string>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>

std::string open_file_dialog_hwnd(const char* filter, HWND hwnd) {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = "";
    
    if (GetOpenFileNameA(&ofn))
        return std::string(fileName);
    return "";
}

std::string open_file_dialog(const char* filter, GLFWwindow* window) {
    return open_file_dialog_hwnd(filter, glfwGetWin32Window(window));
}

// --- Global Variables ---
char pmx_path_buf[256] = "models/taffy/taffy.pmx";
char vmd_path_buf[256] = "motions/TDA.vmd";

Camera* camera = nullptr;
MeshPainter* painter = nullptr;
TriMesh* mesh = nullptr;
VMDAnimation* animation = nullptr;
Stage* stage = nullptr;

bool mouse_left_pressed = false, mouse_right_pressed = false, mouse_middle_pressed = false;
bool needs_redraw = true;
double rotate_sensitivity = 0.2, translate_sensitivity = 0.05;
double last_x, last_y;

// Lighting
std::vector<Light> lights;
glm::vec3 ambient_color(1.0f, 1.0f, 1.0f);
float ambient_strength = 0.4f;

// Shadow Mapping
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// Animation timing
float last_frame_time = 0.0f;
float current_frame = 0.0f;
bool is_playing = true;
bool show_stage = true;
bool enable_motion = true;

// FPS Limiter
int target_fps = 60;
bool limit_fps = true;

// --- Function Declarations ---
void framebuffer_size_callback(GLFWwindow* w, int width, int height);
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

// --- Native Menu ---
// Menu IDs
#define ID_FILE_SELECT_MODEL 1001
#define ID_FILE_SELECT_MOTION 1002

WNDPROC original_wnd_proc = nullptr;

LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_COMMAND) {
        switch (LOWORD(wParam)) {
            case ID_FILE_SELECT_MODEL: {
                std::string path = open_file_dialog_hwnd("PMX Files (*.pmx)\0*.pmx\0All Files (*.*)\0*.*\0", hwnd);
                if (!path.empty()) {
                    strncpy(pmx_path_buf, path.c_str(), 255);
                    load_model(pmx_path_buf);
                }
                return 0;
            }
            case ID_FILE_SELECT_MOTION: {
                std::string path = open_file_dialog_hwnd("VMD Files (*.vmd)\0*.vmd\0All Files (*.*)\0*.*\0", hwnd);
                if (!path.empty()) {
                    strncpy(vmd_path_buf, path.c_str(), 255);
                    load_motion(vmd_path_buf);
                }
                return 0;
            }
        }
    }
    return CallWindowProc(original_wnd_proc, hwnd, uMsg, wParam, lParam);
}

void CreateNativeMenu(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();

    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SELECT_MODEL, "Select Model");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SELECT_MOTION, "Select Motion");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");

    SetMenu(hwnd, hMenu);
    
    // Subclass the window procedure
    original_wnd_proc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MenuWndProc);

    // Force a redraw and layout update to ensure client area is correct
    DrawMenuBar(hwnd);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    
    // Hack: Force a tiny resize to ensure GLFW and Windows agree on the client area size immediately
    RECT rect;
    GetWindowRect(hwnd, &rect);
    SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top + 1, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
}

// --- Initialization ---
void init() {
    camera = new Camera();
    painter = new MeshPainter();
    stage = new Stage(100.0f, 20);
    
    // Initialize Lights
    if (lights.empty()) {
        Light mainLight;
        mainLight.type = LIGHT_DIRECTIONAL;
        mainLight.direction = glm::vec3(-0.5f, -1.0f, -0.5f);
        mainLight.color = glm::vec3(1.0f);
        mainLight.intensity = 0.5f;
        lights.push_back(mainLight);
    }

    load_model(pmx_path_buf);
    load_motion(vmd_path_buf);

    // Configure Shadow Map FBO
    glGenFramebuffers(1, &depthMapFBO);
    
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void load_model(const std::string& path) {
    if (mesh) {
        delete mesh;
        mesh = nullptr;
    }
    // Re-create painter to clear old mesh data
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


// --- Render Loop ---
void display() {
    // 1. Shadow Pass
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 100.0f;
    
    // Use the first light as the shadow caster
    glm::vec3 lightPos = glm::vec3(-2.0f, 4.0f, -1.0f); // Default
    if (!lights.empty()) {
        if (lights[0].type == LIGHT_DIRECTIONAL) {
             lightPos = -lights[0].direction * 20.0f; // Simulate a position for directional light
             lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
        } else {
             lightPos = lights[0].position;
             lightProjection = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        }
    } else {
         lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
    }
    
    lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;
    
    // Cull front faces for shadow mapping to fix peter panning
    // Removed global culling override to support double-sided materials
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);
    
    if (show_stage && stage) stage->draw_shadow(lightSpaceMatrix);
    painter->draw_shadow(lightSpaceMatrix);
    
    // glCullFace(GL_BACK); // Reset culling
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. Lighting Pass
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (show_stage && stage) stage->draw(camera, lights, ambient_color, ambient_strength, depthMap, lightSpaceMatrix);
    painter->draw_meshes(camera, lights, ambient_color, ambient_strength, depthMap, lightSpaceMatrix);
    painter->draw_light_gizmos(camera, lights);
}

// --- Callbacks ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { 
    if (height == 0) height = 1; // Prevent divide by zero
    glViewport(0, 0, width, height); 
    // Check if camera is initialized before accessing it
    if (camera) {
        camera->aspect = (float)width / (float)height;
        // camera->update_camera_vectors(); // Removed: This resets rotation to Euler angles
    }
    needs_redraw = true; 
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    double deltaX = xpos - last_x;
    double deltaY = ypos - last_y;
    
    if (mouse_left_pressed) { 
        if (camera) camera->orbit(deltaX * rotate_sensitivity * 2.0f, deltaY * rotate_sensitivity * 2.0f);
        needs_redraw = true; 
    }
    if (mouse_right_pressed) { 
        if (camera) camera->pan(deltaX * translate_sensitivity, deltaY * translate_sensitivity);
        needs_redraw = true; 
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
        needs_redraw = true;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        bool changed = true;
        if (camera) camera->handle_keys(key, action);
        
        switch (key) {
            case GLFW_KEY_Q: {
                glm::vec3 rot = mesh->get_rotation();
                rot.z += 5.0f;
                mesh->set_rotation(rot);
                break;
            }
            case GLFW_KEY_E: {
                glm::vec3 rot = mesh->get_rotation();
                rot.z -= 5.0f;
                mesh->set_rotation(rot);
                break;
            }
            case GLFW_KEY_1: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
            case GLFW_KEY_2: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
            case GLFW_KEY_R: reset(); break;
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
            default: changed = false; break;
        }
        if (changed) needs_redraw = true;
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
    printf("  Q, E:        Rotate model on Z-axis\n");
    printf("  1:           Solid mode\n");
    printf("  2:           Wireframe mode\n");
    printf("  R:           Reset all transformations\n");
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
    needs_redraw = true;
}

int main(int argc, char** argv) {
    timeBeginPeriod(1);

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
    glfwSwapInterval(0); // Disable VSync to allow manual FPS limiting

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

    CreateNativeMenu(window);
    // Process pending events (like WM_SIZE) to ensure GLFW updates its window size/content scale
    glfwPollEvents();

    print_help();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);

    last_frame_time = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - last_frame_time;
        last_frame_time = currentTime;

        if (enable_motion) {
            if (is_playing && animation) {
                // VMD 标准帧率为 30 FPS
                current_frame += deltaTime * 30.0f; 
                if (current_frame >= animation->get_duration()) {
                    current_frame = 0.0f;
                }
                animation->update(current_frame, mesh);
                mesh->update_bone_matrices();
            }
        } else {
            // 不启用动画时，重置模型为 T-Pose
            if (mesh) {
                mesh->reset_pose();
                mesh->update_bone_matrices();
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("MMD Viewer Controls");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            
            ImGui::Separator();
            ImGui::Checkbox("Show Stage", &show_stage);
            ImGui::Checkbox("Enable Motion", &enable_motion);
            if (enable_motion) {
                ImGui::Checkbox("Play Animation", &is_playing);
            }

            ImGui::Separator();
            ImGui::Text("Performance");
            ImGui::Checkbox("Limit FPS", &limit_fps);
            if (limit_fps) {
                ImGui::SliderInt("Target FPS", &target_fps, 10, 240);
            }

            ImGui::Separator();
            ImGui::Text("Lighting");
            ImGui::ColorEdit3("Ambient Color", (float*)&ambient_color);
            ImGui::SliderFloat("Ambient Strength", &ambient_strength, 0.0f, 1.0f);

            if (ImGui::CollapsingHeader("Lights")) {
                if (ImGui::Button("Add Point Light")) {
                    if (lights.size() < 16) {
                        Light l;
                        l.type = LIGHT_POINT;
                        l.position = glm::vec3(0.0f, 10.0f, 0.0f);
                        lights.push_back(l);
                    }
                }
                
                for (int i = 0; i < lights.size(); ++i) {
                    ImGui::PushID(i);
                    if (ImGui::TreeNode(("Light " + std::to_string(i)).c_str())) {
                        ImGui::Checkbox("Enabled", &lights[i].enabled);
                        const char* types[] = { "Directional", "Point" };
                        ImGui::Combo("Type", &lights[i].type, types, IM_ARRAYSIZE(types));
                        
                        if (lights[i].type == LIGHT_DIRECTIONAL) {
                            ImGui::DragFloat3("Direction", (float*)&lights[i].direction, 0.1f);
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
                            ImGui::TreePop();
                            ImGui::PopID();
                            i--; // Adjust index
                            continue; 
                        }
                        
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            ImGui::End();
        }

        ImGui::Render();

        display();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        
        glfwPollEvents();

        // FPS Limiter
        if (limit_fps && target_fps > 0) {
            float frameTime = (float)glfwGetTime() - currentTime;
            float targetFrameTime = 1.0f / (float)target_fps;
            if (frameTime < targetFrameTime) {
                float sleepTime = targetFrameTime - frameTime;
                if (sleepTime > 0.002f) { // Sleep if more than 2ms remaining
                     Sleep((DWORD)(sleepTime * 1000.0f - 1.0f)); 
                }
                // Busy wait for the rest
                while ((float)glfwGetTime() - currentTime < targetFrameTime) {
                    // spin
                }
            }
        }
    }

    clean_up();
    glfwTerminate();
    timeEndPeriod(1);
    return 0;
}