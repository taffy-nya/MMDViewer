#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"
#include "VMDAnimation.h"
#include "Stage.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <vector>
#include <string>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <commdlg.h>

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
#endif

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
double rotate_sensitivity = 0.2, translate_sensitivity = 0.02;
double last_x, last_y;
glm::vec3 light_pos(0.0f, 50.0f, 50.0f);

// Animation timing
float last_frame_time = 0.0f;
float current_frame = 0.0f;
bool is_playing = true;
bool show_stage = true;
bool enable_motion = true;

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
#ifdef _WIN32
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
#endif

// --- Initialization ---
void init() {
    camera = new Camera();
    painter = new MeshPainter();
    stage = new Stage(100.0f, 20);
    
    load_model(pmx_path_buf);
    load_motion(vmd_path_buf);

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (show_stage && stage) stage->draw(camera);
    painter->draw_meshes(camera, light_pos);
}

// --- Callbacks ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { 
    if (height == 0) height = 1; // Prevent divide by zero
    glViewport(0, 0, width, height); 
    // Check if camera is initialized before accessing it
    if (camera) {
        camera->aspect = (float)width / (float)height;
        camera->update_camera_vectors(); // Re-calculate projection if needed, though aspect is used in getProjectionMatrix
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
    printf("  Left Drag:   Rotate model\n");
    printf("  Right Drag:  Translate model\n");
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
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "MMD Model Viewer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

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

#ifdef _WIN32
    CreateNativeMenu(window);
    // Process pending events (like WM_SIZE) to ensure GLFW updates its window size/content scale
    glfwPollEvents();
#endif

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

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
                // 30 FPS is standard for VMD
                current_frame += deltaTime * 30.0f; 
                if (current_frame >= animation->get_duration()) {
                    current_frame = 0.0f; // Loop
                }
                animation->update(current_frame, mesh);
                mesh->update_bone_matrices();
            }
        } else {
            // Reset to T-Pose if motion is disabled
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

            ImGui::End();
        }

        ImGui::Render();

        display();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        
        glfwPollEvents();
    }

    clean_up();
    glfwTerminate();
    return 0;
}
