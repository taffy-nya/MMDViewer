#include "Angel.h"
#include "TriMesh.h"
#include "stb_image.h"

#include <vector>
#include <string>

// --- 全局变量 ---

const std::string pmx_path = "models/taffy/taffy.pmx";

glm::vec3 scaleTheta(1.0f), rotateTheta(0.0f), translateTheta(0.0f);
double brightness = 1.0;
bool mouseLeftPressed = false, mouseRightPressed = false, mouseMiddlePressed = false;
double rotateSensitivity = 0.2, translateSensitivity = 0.02, brightnessSensitivity = 0.01;
double lastX, lastY;
glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 35.0f), cameraFront = glm::vec3(0.0f, 0.0f, -1.0f), cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraZoomSensitivity = 1.0f;
glm::vec3 lightPos(0.0f, 50.0f, 50.0f);

struct OpenGLObject
{
    GLuint vao = 0, vbo = 0, ebo = 0, program = 0;
    
    // Uniform locations are now cached here
    GLuint modelLocation, viewLocation, projectionLocation;
    GLuint objectColorLocation, lightColorLocation, lightPosLocation, viewPosLocation;
    GLuint brightnessLocation, hasTextureLocation, shininessLocation;
    GLuint edgeSizeLocation, edgeColorLocation; // For edge shader
    GLuint textureSamplerLocation, toonSamplerLocation;
};

OpenGLObject mmdObject;
OpenGLObject edgeObject;
// std::vector<GLuint> toonTextures;
TriMesh *mesh = new TriMesh();

// --- 函数声明 ---
void framebuffer_size_callback(GLFWwindow* w, int width, int height);
void key_callback(GLFWwindow* w, int k, int s, int a, int m);
void mouse_button_callback(GLFWwindow* w, int b, int a, int m);
void cursor_position_callback(GLFWwindow* w, double x, double y);
void scroll_callback(GLFWwindow* w, double x, double y);
void bindObjectAndData(TriMesh* m, OpenGLObject& o, const std::string& vs, const std::string& fs);
void init();
void display();
void cleanUp();
void reset();
void printHelp();

// --- OpenGL与数据绑定 ---
void bindObjectAndData(TriMesh* mesh, OpenGLObject& object, const std::string& vshader, const std::string& fshader) {
    glGenVertexArrays(1, &object.vao);
    glBindVertexArray(object.vao);
    const auto &p = mesh->getVertexPositions();
    const auto &n = mesh->getVertexNormals();
    const auto &u = mesh->getVertexUVs();
    if (p.empty() || p.size() != n.size() || p.size() != u.size()) { std::cerr << "Vertex attribute mismatch!" << std::endl; return; }
    size_t p_size = p.size()*sizeof(glm::vec3), n_size = n.size()*sizeof(glm::vec3), u_size = u.size()*sizeof(glm::vec2);
    glGenBuffers(1, &object.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
    glBufferData(GL_ARRAY_BUFFER, p_size+n_size+u_size, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, p_size, p.data());
    glBufferSubData(GL_ARRAY_BUFFER, p_size, n_size, n.data());
    glBufferSubData(GL_ARRAY_BUFFER, p_size+n_size, u_size, u.data());
    glGenBuffers(1, &object.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->getFaces().size()*sizeof(vec3u), mesh->getFaces().data(), GL_STATIC_DRAW);
    object.program = InitShader(vshader.c_str(), fshader.c_str());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(p_size)); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(p_size+n_size)); glEnableVertexAttribArray(2);
}

// --- 初始化 ---
void init() {
    mesh->readPmx(pmx_path);
    std::string base_path = "";
    size_t last_slash = pmx_path.rfind('/');
    if(last_slash == std::string::npos) last_slash = pmx_path.rfind('\\');
    if(last_slash != std::string::npos) base_path = pmx_path.substr(0, last_slash + 1);
    mesh->loadOpenGLTextures(base_path);

    // stbi_set_flip_vertically_on_load(true);
    // toonTextures.resize(11); // Use 11 for safety, indices are 1-10
    // for (int i = 1; i <= 10; ++i) {
    //     std::string toonPath = std::string("models/toons/toon") + (i < 10 ? "0" : "") + std::to_string(i) + ".bmp";
    //     int w, h, c;
    //     unsigned char* data = stbi_load(toonPath.c_str(), &w, &h, &c, 0);
    //     if (data) {
    //         glGenTextures(1, &toonTextures[i]);
    //         glBindTexture(GL_TEXTURE_2D, toonTextures[i]);
    //         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //         stbi_image_free(data);
    //     } else { std::cerr << "Failed to load toon texture: " << toonPath << std::endl; }
    // }

    // --- 绑定主模型并缓存 Uniform Locations ---
    bindObjectAndData(mesh, mmdObject, "shaders/vshader.glsl", "shaders/fshader.glsl");
    mmdObject.modelLocation = glGetUniformLocation(mmdObject.program, "model");
    mmdObject.viewLocation = glGetUniformLocation(mmdObject.program, "view");
    mmdObject.projectionLocation = glGetUniformLocation(mmdObject.program, "projection");
    mmdObject.objectColorLocation = glGetUniformLocation(mmdObject.program, "objectColor");
    mmdObject.lightColorLocation = glGetUniformLocation(mmdObject.program, "lightColor");
    mmdObject.lightPosLocation = glGetUniformLocation(mmdObject.program, "lightPos");
    mmdObject.viewPosLocation = glGetUniformLocation(mmdObject.program, "viewPos");
    mmdObject.brightnessLocation = glGetUniformLocation(mmdObject.program, "brightness");
    mmdObject.hasTextureLocation = glGetUniformLocation(mmdObject.program, "hasTexture");
    mmdObject.shininessLocation = glGetUniformLocation(mmdObject.program, "shininess");
    mmdObject.textureSamplerLocation = glGetUniformLocation(mmdObject.program, "textureSampler");
    mmdObject.toonSamplerLocation = glGetUniformLocation(mmdObject.program, "toonSampler");

    // --- 编译轮廓线着色器并缓存 Uniform Locations ---
    edgeObject.program = InitShader("shaders/vshader_edge.glsl", "shaders/fshader_edge.glsl");
    edgeObject.modelLocation = glGetUniformLocation(edgeObject.program, "model");
    edgeObject.viewLocation = glGetUniformLocation(edgeObject.program, "view");
    edgeObject.projectionLocation = glGetUniformLocation(edgeObject.program, "projection");
    edgeObject.edgeSizeLocation = glGetUniformLocation(edgeObject.program, "edge_size");
    edgeObject.edgeColorLocation = glGetUniformLocation(edgeObject.program, "edge_color");

    glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// --- 渲染循环 ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 model(1.0f), view(1.0f), projection(1.0f);
    model = glm::translate(model, translateTheta);
    model = glm::rotate(model, glm::radians(rotateTheta.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotateTheta.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotateTheta.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scaleTheta);
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);
    glBindVertexArray(mmdObject.vao);
    
    // Pass 1: Outline
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glUseProgram(edgeObject.program);
    glUniformMatrix4fv(edgeObject.modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(edgeObject.viewLocation, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(edgeObject.projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
    unsigned int face_offset = 0;
    for (const auto& mat : mesh->getMaterials()) {
        glUniform1f(edgeObject.edgeSizeLocation, mat.edge_size * 0.03f);
        glUniform4fv(edgeObject.edgeColorLocation, 1, glm::value_ptr(mat.edge_color));
        glDrawElements(GL_TRIANGLES, mat.num_faces, GL_UNSIGNED_INT, (void*)(face_offset*sizeof(unsigned int)));
        face_offset += mat.num_faces;
    }

    // Pass 2: Main Model
    glCullFace(GL_BACK);
    glUseProgram(mmdObject.program);
    glUniformMatrix4fv(mmdObject.modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mmdObject.viewLocation, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mmdObject.projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(mmdObject.lightColorLocation, 1, glm::value_ptr(glm::vec3(1.0f)));
    glUniform3fv(mmdObject.lightPosLocation, 1, glm::value_ptr(lightPos));
    glUniform3fv(mmdObject.viewPosLocation, 1, glm::value_ptr(cameraPos));
    glUniform1f(mmdObject.brightnessLocation, (float)brightness);
    glUniform1i(mmdObject.textureSamplerLocation, 0);
    glUniform1i(mmdObject.toonSamplerLocation, 1);
    face_offset = 0;
    for (const auto& mat : mesh->getMaterials()) {
        glUniform4fv(mmdObject.objectColorLocation, 1, glm::value_ptr(mat.diffuse_color));
        glUniform1f(mmdObject.shininessLocation, mat.shininess); // Set shininess for each material
        if (mat.has_texture) {
            glUniform1i(mmdObject.hasTextureLocation, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh->getTextures()[mat.texture_index].gl_texture_id);
        } else {
            glUniform1i(mmdObject.hasTextureLocation, 0);
        }
        // if (mat.toon_texture_index >= 0 && mat.toon_texture_index < toonTextures.size()) {
        //     glActiveTexture(GL_TEXTURE1);
        //     glBindTexture(GL_TEXTURE_2D, toonTextures[mat.toon_texture_index]);
        // }
        glDrawElements(GL_TRIANGLES, mat.num_faces, GL_UNSIGNED_INT, (void*)(face_offset*sizeof(unsigned int)));
        face_offset += mat.num_faces;
    }
    
    glDisable(GL_CULL_FACE);
    glBindVertexArray(0);
}

// --- 回调函数与主循环 ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    double deltaX = xpos - lastX;
    double deltaY = ypos - lastY;
    if (mouseLeftPressed) { rotateTheta.y += deltaX * rotateSensitivity; rotateTheta.x += deltaY * rotateSensitivity; }
    if (mouseRightPressed) { translateTheta.x += deltaX * translateSensitivity; translateTheta.y -= deltaY * translateSensitivity; }
    if (mouseMiddlePressed) { brightness -= deltaY * brightnessSensitivity; brightness = glm::clamp(brightness, 0.0, 10.0); }
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
    cameraPos += (float)yoffset * cameraFront * cameraZoomSensitivity;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    float cameraSpeed = 0.5f;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_W: cameraPos += cameraSpeed * cameraFront; break;
            case GLFW_KEY_S: cameraPos -= cameraSpeed * cameraFront; break;
            case GLFW_KEY_A: cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; break;
            case GLFW_KEY_D: cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; break;
            case GLFW_KEY_Q: rotateTheta.z += 5.0f; break;
            case GLFW_KEY_E: rotateTheta.z -= 5.0f; break;
            case GLFW_KEY_1: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
            case GLFW_KEY_2: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
            case GLFW_KEY_R: reset(); break;
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
        }
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
    delete mesh;
    glDeleteVertexArrays(1, &mmdObject.vao);
    glDeleteBuffers(1, &mmdObject.vbo);
    glDeleteBuffers(1, &mmdObject.ebo);
    glDeleteProgram(mmdObject.program);
}

void reset() {
    scaleTheta = glm::vec3(1.0, 1.0, 1.0);
    rotateTheta = glm::vec3(0.0, 0.0, 0.0);
    translateTheta = glm::vec3(0.0, 0.0, 0.0);
    brightness = 1.0;
    cameraPos = glm::vec3(0.0f, 10.0f, 35.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
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

    while (!glfwWindowShouldClose(window)) {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanUp();
    glfwTerminate();
    return 0;
}