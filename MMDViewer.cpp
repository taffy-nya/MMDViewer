#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"
#include "VMDAnimation.h"

#include <vector>
#include <string>

// --- Global Variables ---
const std::string pmx_path = "models/taffy/taffy.pmx";
// const std::string vmd_path = "motions/TDA.vmd";
const std::string vmd_path = "motions/daisukeEvolution.vmd";

Camera* camera = nullptr;
MeshPainter* painter = nullptr;
TriMesh* mesh = nullptr;
VMDAnimation* animation = nullptr;

double brightness = 1.0;
bool mouseLeftPressed = false, mouseRightPressed = false, mouseMiddlePressed = false;
bool needsRedraw = true;
double rotateSensitivity = 0.2, translateSensitivity = 0.02, brightnessSensitivity = 0.01;
double lastX, lastY;
glm::vec3 lightPos(0.0f, 50.0f, 50.0f);

// Animation timing
float lastFrameTime = 0.0f;
float currentFrame = 0.0f;
bool isPlaying = true;

// --- Function Declarations ---
void framebuffer_size_callback(GLFWwindow* w, int width, int height);
void key_callback(GLFWwindow* w, int k, int s, int a, int m);
void mouse_button_callback(GLFWwindow* w, int b, int a, int m);
void cursor_position_callback(GLFWwindow* w, double x, double y);
void scroll_callback(GLFWwindow* w, double x, double y);
void init();
void display();
void cleanUp();
void reset();
void printHelp();

// --- Initialization ---
void init() {
    camera = new Camera();
    painter = new MeshPainter();
    mesh = new TriMesh();

    mesh->readPmx(pmx_path);
    
    std::string base_path = "";
    size_t last_slash = pmx_path.rfind('/');
    if(last_slash == std::string::npos) last_slash = pmx_path.rfind('\\');
    if(last_slash != std::string::npos) base_path = pmx_path.substr(0, last_slash + 1);
    mesh->loadOpenGLTextures(base_path);

    animation = new VMDAnimation();
    if (!animation->load(vmd_path)) {
        std::cerr << "Failed to load VMD: " << vmd_path << std::endl;
    } else {
        std::cout << "Loaded VMD: " << vmd_path << " (" << animation->getDuration() << " frames)" << std::endl;
    }

    painter->addMesh(mesh, "shaders/vshader.glsl", "shaders/fshader.glsl", "shaders/vshader_edge.glsl", "shaders/fshader_edge.glsl");

    glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// --- Render Loop ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    painter->drawMeshes(camera, lightPos, (float)brightness);
}

// --- Callbacks ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { 
    if (height == 0) height = 1; // Prevent divide by zero
    glViewport(0, 0, width, height); 
    if (camera) {
        camera->aspect = (float)width / (float)height;
        camera->updateCameraVectors(); // Re-calculate projection if needed, though aspect is used in getProjectionMatrix
    }
    needsRedraw = true; 
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    double deltaX = xpos - lastX;
    double deltaY = ypos - lastY;
    
    if (mouseLeftPressed) { 
        glm::vec3 rot = mesh->getRotation();
        rot.y += deltaX * rotateSensitivity; 
        rot.x += deltaY * rotateSensitivity; 
        mesh->setRotation(rot);
        needsRedraw = true; 
    }
    if (mouseRightPressed) { 
        glm::vec3 trans = mesh->getTranslation();
        trans.x += deltaX * translateSensitivity; 
        trans.y -= deltaY * translateSensitivity; 
        mesh->setTranslation(trans);
        needsRedraw = true; 
    }
    if (mouseMiddlePressed) { 
        brightness -= deltaY * brightnessSensitivity; 
        brightness = glm::clamp(brightness, 0.0, 10.0); 
        needsRedraw = true; 
    }
    lastX = xpos; lastY = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        glfwGetCursorPos(window, &lastX, &lastY);
        if (button == GLFW_MOUSE_BUTTON_LEFT) mouseLeftPressed = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) mouseRightPressed = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) mouseMiddlePressed = true;
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) mouseLeftPressed = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) mouseRightPressed = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) mouseMiddlePressed = false;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (camera) {
        camera->handleScroll(yoffset);
        needsRedraw = true;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        bool changed = true;
        if (camera) camera->handleKeys(key, action);
        
        switch (key) {
            case GLFW_KEY_Q: {
                glm::vec3 rot = mesh->getRotation();
                rot.z += 5.0f;
                mesh->setRotation(rot);
                break;
            }
            case GLFW_KEY_E: {
                glm::vec3 rot = mesh->getRotation();
                rot.z -= 5.0f;
                mesh->setRotation(rot);
                break;
            }
            case GLFW_KEY_1: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
            case GLFW_KEY_2: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
            case GLFW_KEY_R: reset(); break;
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
            default: changed = false; break;
        }
        if (changed) needsRedraw = true;
    }
}

void printHelp() {
    printf("--- MMD Model Viewer Controls ---\n");
    printf("Mouse:\n");
    printf("  Left Drag:   Rotate model\n");
    printf("  Right Drag:  Translate model\n");
    printf("  Middle Drag: Adjust brightness\n");
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

void cleanUp() {
    if (painter) delete painter;
    if (mesh) delete mesh;
    if (camera) delete camera;
    if (animation) delete animation;
}

void reset() {
    if (mesh) {
        mesh->setScale(glm::vec3(1.0f));
        mesh->setRotation(glm::vec3(0.0f));
        mesh->setTranslation(glm::vec3(0.0f));
    }
    brightness = 1.0;
    if (camera) camera->reset();
    needsRedraw = true;
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
    printHelp();

    lastFrameTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        if (isPlaying && animation) {
            // 30 FPS is standard for VMD
            currentFrame += deltaTime * 30.0f; 
            if (currentFrame >= animation->getDuration()) {
                currentFrame = 0.0f; // Loop
            }
            animation->update(currentFrame, mesh);
            mesh->updateBoneMatrices();
            needsRedraw = true;
        }

        if (needsRedraw) {
            display();
            glfwSwapBuffers(window);
            needsRedraw = false;
        }
        glfwPollEvents();
    }

    cleanUp();
    glfwTerminate();
    return 0;
}
