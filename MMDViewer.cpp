#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"
#include "VMDAnimation.h"

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
#endif

// --- Global Variables ---
char pmx_path_buf[256] = "models/taffy/taffy.pmx";
char vmd_path_buf[256] = "motions/TDA.vmd";

Camera* camera = nullptr;
MeshPainter* painter = nullptr;
TriMesh* mesh = nullptr;
VMDAnimation* animation = nullptr;

bool mouse_left_pressed = false, mouse_right_pressed = false, mouse_middle_pressed = false;
bool needs_redraw = true;
double rotate_sensitivity = 0.2, translate_sensitivity = 0.02;
double last_x, last_y;
glm::vec3 light_pos(0.0f, 50.0f, 50.0f);

// Animation timing
float last_frame_time = 0.0f;
float current_frame = 0.0f;
bool is_playing = true;

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

// --- Initialization ---
void init() {
    camera = new Camera();
    painter = new MeshPainter();
    
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
    painter->draw_meshes(camera, light_pos);
}

// --- Callbacks ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { 
    if (height == 0) height = 1; // Prevent divide by zero
    glViewport(0, 0, width, height); 
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
        glm::vec3 rot = mesh->get_rotation();
        rot.y += deltaX * rotate_sensitivity; 
        rot.x += deltaY * rotate_sensitivity; 
        mesh->set_rotation(rot);
        needs_redraw = true; 
    }
    if (mouse_right_pressed) { 
        glm::vec3 trans = mesh->get_translation();
        trans.x += deltaX * translate_sensitivity; 
        trans.y -= deltaY * translate_sensitivity; 
        mesh->set_translation(trans);
        needs_redraw = true; 
    }
    last_x = xpos; last_y = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
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

    GLFWwindow* window = glfwCreateWindow(1000, 1000, "MMD Model Viewer", NULL, NULL);
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

        if (is_playing && animation) {
            // 30 FPS is standard for VMD
            current_frame += deltaTime * 30.0f; 
            if (current_frame >= animation->get_duration()) {
                current_frame = 0.0f; // Loop
            }
            animation->update(current_frame, mesh);
            mesh->update_bone_matrices();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Select Model")) {
#ifdef _WIN32
                    std::string path = open_file_dialog("PMX Files (*.pmx)\0*.pmx\0All Files (*.*)\0*.*\0", window);
                    if (!path.empty()) {
                        strncpy(pmx_path_buf, path.c_str(), sizeof(pmx_path_buf) - 1);
                        load_model(pmx_path_buf);
                    }
#endif
                }
                if (ImGui::MenuItem("Select Motion")) {
#ifdef _WIN32
                    std::string path = open_file_dialog("VMD Files (*.vmd)\0*.vmd\0All Files (*.*)\0*.*\0", window);
                    if (!path.empty()) {
                        strncpy(vmd_path_buf, path.c_str(), sizeof(vmd_path_buf) - 1);
                        load_motion(vmd_path_buf);
                    }
#endif
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        {
            ImGui::Begin("MMD Viewer Controls");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            
            ImGui::Separator();
            ImGui::Checkbox("Play Animation", &is_playing);

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
