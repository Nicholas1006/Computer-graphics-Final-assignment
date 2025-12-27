#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <render/shader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
// ====================
// RENDER CONTROLS
// ====================
static bool renderRobot = true;
static bool renderSky = false;
static bool renderGround = true;

// ====================
// GLOBAL VARIABLES
// ====================
static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;
static bool firstMouse = true;
static float lastX = windowWidth / 2.0f;
static float lastY = windowHeight / 2.0f;

// Camera
static glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 500.0f);  // Increased height and distance
static glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
static glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
static float cameraSpeed = 100.0f;
static float yaw = -90.0f;
static float pitch = -10.0f;  // Looking slightly down
static float FoV = 45.0f;
static float zNear = 0.1f;
static float zFar = 5000.0f;

// Lighting
static glm::vec3 lightIntensity(5e6f, 5e6f, 5e6f);
static glm::vec3 lightPosition(0.0f, 500.0f, 0.0f);

// Animation
static bool playAnimation = true;
static float playbackSpeed = 1.0f;

// ====================
// FUNCTION DECLARATIONS
// ====================
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow* window, float deltaTime);


void checkGLError(const char* context) {
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        std::cout << "OpenGL Error at " << context << ": ";
        switch(error) {
            case GL_INVALID_ENUM: std::cout << "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: std::cout << "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: std::cout << "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY: std::cout << "GL_OUT_OF_MEMORY"; break;
            default: std::cout << "Unknown error " << error; break;
        }
        std::cout << std::endl;
        error = glGetError();
    }
}

int pow(int base, int exp) { 
	if(exp==0) {
		return 1;
	}
	else{
		return base * pow(base, exp - 1);
	}
}

int abs(int x) {
	if(x<0) {
		return -x;
	}
	else {
		return x;
	}
}

// ====================
// SIMPLE SHADER CREATION
// ====================
GLuint createSimpleShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 MVP;
        void main() {
            gl_Position = MVP * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec3 FragColor;
        void main() {
            FragColor = vec3(1.0, 0.5, 0.2); // Orange color
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Simple Vertex shader compilation failed: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Simple Fragment shader compilation failed: " << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Simple Shader program linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

// ====================
// TEST CUBE
// ====================
struct TestCube {
    GLuint vao, vbo, ebo;
    GLuint shader;
    int indexCount;
    
    void initialize() {
        std::cout << "Initializing test cube..." << std::endl;
        
        // Cube vertices
        float vertices[] = {
            // positions
            -50.0f, -50.0f, -50.0f,
             50.0f, -50.0f, -50.0f,
             50.0f,  50.0f, -50.0f,
            -50.0f,  50.0f, -50.0f,
            -50.0f, -50.0f,  50.0f,
             50.0f, -50.0f,  50.0f,
             50.0f,  50.0f,  50.0f,
            -50.0f,  50.0f,  50.0f
        };
        
        // Cube indices
        unsigned int indices[] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            0, 4, 7, 7, 3, 0,
            1, 5, 6, 6, 2, 1,
            3, 2, 6, 6, 7, 3,
            0, 1, 5, 5, 4, 0
        };
        
        indexCount = 36;
        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(0);
        
        // Create simple shader
        shader = createSimpleShader();
        std::cout << "Test cube shader ID: " << shader << std::endl;
    }
    
    void render(const glm::mat4& viewProj) {
        if (!renderRobot) return;
        glUseProgram(shader);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 100.0f, 0.0f));
        glm::mat4 mvp = viewProj * model;
        
        GLuint mvpLoc = glGetUniformLocation(shader, "MVP");
        if (mvpLoc != -1) {
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        }
        
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void cleanup() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteProgram(shader);
    }
};

// ====================
// SIMPLE GROUND
// ====================
struct SimpleGround {
    GLuint vao, vbo, uvVbo, ebo;
    GLuint shader;
    GLuint textureID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;
    
    void initialize() {
        std::cout << "Initializing textured ground..." << std::endl;
        
        // Ground vertices (two triangles forming a quad)
        float vertices[] = {
            // positions
            -1000.0f, 0.0f, -1000.0f,
             1000.0f, 0.0f, -1000.0f,
             1000.0f, 0.0f,  1000.0f,
            -1000.0f, 0.0f,  1000.0f
        };
        
        // Texture coordinates (with tiling)
        float uvCoords[] = {
            0.0f, 0.0f,
            20.0f, 0.0f,  // Tiled 20 times
            20.0f, 20.0f,
            0.0f, 20.0f
        };
        
        // Indices for two triangles
        unsigned int indices[] = {
            0, 1, 2,
            0, 2, 3
        };
        
        // Create and bind VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        // Vertex buffer
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // UV buffer
        glGenBuffers(1, &uvVbo);
        glBindBuffer(GL_ARRAY_BUFFER, uvVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uvCoords), uvCoords, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        
        // Index buffer
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glBindVertexArray(0);
        
        // Create shader for textured ground
        const char* groundVert = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            uniform mat4 MVP;
            
            void main() {
                gl_Position = MVP * vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        
        const char* groundFrag = R"(
            #version 330 core
            in vec2 TexCoord;
            out vec3 FragColor;
            
            uniform sampler2D groundTexture;
            
            void main() {
                FragColor = texture(groundTexture, TexCoord).rgb;
                // Add some variation to make it look more natural
                FragColor *= vec3(0.8, 1.0, 0.8); // Slightly greener tint
            }
        )";
        
        // Compile shaders
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &groundVert, NULL);
        glCompileShader(vertexShader);
        
        GLint success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "Ground Vertex shader compilation failed: " << infoLog << std::endl;
        }
        
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &groundFrag, NULL);
        glCompileShader(fragmentShader);
        
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "Ground Fragment shader compilation failed: " << infoLog << std::endl;
        }
        
        // Link shader program
        shader = glCreateProgram();
        glAttachShader(shader, vertexShader);
        glAttachShader(shader, fragmentShader);
        glLinkProgram(shader);
        
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 512, NULL, infoLog);
            std::cout << "Ground Shader program linking failed: " << infoLog << std::endl;
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        // Get uniform locations
        mvpMatrixID = glGetUniformLocation(shader, "MVP");
        textureSamplerID = glGetUniformLocation(shader, "groundTexture");
        
        // Load ground texture
        textureID = loadGroundTexture();
        
        std::cout << "Textured ground initialized. Shader ID: " << shader 
                  << ", Texture ID: " << textureID << std::endl;
    }
    
    GLuint loadGroundTexture() {
        // Try to load grass texture
        const char* texturePath = "../assignment1/model/ground/ground.jpg";  // Adjust path as needed
        
        int width, height, channels;
        unsigned char* data = stbi_load(texturePath, &width, &height, &channels, 0);
        
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        if (data) {
            std::cout << "Loaded grass texture: " << texturePath 
                      << " (" << width << "x" << height << ", channels: " << channels << ")" << std::endl;
            
            GLenum format = GL_RGB;
            if (channels == 4) format = GL_RGBA;
            else if (channels == 1) format = GL_RED;
            
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            // Set texture parameters for tiling
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            stbi_image_free(data);
        } else {
            std::cout << "Failed to load grass texture. Creating procedural grass texture." << std::endl;
            createProceduralGrassTexture(texture);
        }
        
        return texture;
    }
    
    void createProceduralGrassTexture(GLuint texture) {
        const int width = 256;
        const int height = 256;
        std::vector<unsigned char> pixels(width * height * 3);
        
        // Create a procedural grass pattern
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                
                // Base grass color with variation
                float noise = 0.2f * ((x * 17 + y * 23) % 100) / 100.0f;
                
                pixels[idx] = static_cast<unsigned char>(40 + 50 * noise);      // R
                pixels[idx + 1] = static_cast<unsigned char>(160 + 40 * noise);  // G
                pixels[idx + 2] = static_cast<unsigned char>(40 + 30 * noise);   // B
                
                // Add some darker patches for variation
                if (((x / 16) + (y / 16)) % 4 == 0) {
                    pixels[idx] = static_cast<unsigned char>(pixels[idx] * 0.8f);
                    pixels[idx + 1] = static_cast<unsigned char>(pixels[idx + 1] * 0.8f);
                }
            }
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        std::cout << "Created procedural grass texture." << std::endl;
    }
    
    void render(const glm::mat4& viewProj) {
        if (!renderGround) return;
        
        glUseProgram(shader);
        glBindVertexArray(vao);
        
        // Set MVP matrix
        glm::mat4 mvp = viewProj; // Identity model matrix
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, glm::value_ptr(mvp));
        
        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(textureSamplerID, 0);
        
        // Draw ground
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glBindVertexArray(0);
    }
    
    void cleanup() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &uvVbo);
        glDeleteBuffers(1, &ebo);
        glDeleteTextures(1, &textureID);
        glDeleteProgram(shader);
    }
};

static GLuint LoadTextureTileBox(const char *texture_file_path) {
    int w, h, channels;
    uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    GLuint texture;
    glGenTextures(1, &texture);  
    glBindTexture(GL_TEXTURE_2D, texture);  

    // To tile textures on a box, we set wrapping to repeat
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (img) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture " << texture_file_path << std::endl;
    }
    stbi_image_free(img);

    return texture;
}
// MAIN FUNCTION
// ====================
int main(void) {
    // Initialise GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Debug Test - Simple Scene", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to open a GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Load OpenGL functions
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL context." << std::endl;
        return -1;
    }

    std::cout << "\n=== OpenGL Info ===" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl << std::endl;

    // Set viewport
    glViewport(0, 0, windowWidth, windowHeight);
    
    // OpenGL settings
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Disable culling for debugging

    // Initialize test objects
    TestCube testCube;
    SimpleGround ground;
    
    std::cout << "=== Initializing Test Scene ===" << std::endl;
    testCube.initialize();
    ground.initialize();



    
    // Update camera front based on initial pitch/yaw
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    // Set up callbacks AFTER window creation
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Time tracking
    static double lastTime = glfwGetTime();
    float fTime = 0.0f;
    unsigned long frames = 0;

    std::cout << "\n=== Starting Render Loop ===" << std::endl;
    std::cout << "Camera position: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - WASD: Move camera" << std::endl;
    std::cout << "  - Mouse: Look around" << std::endl;
    std::cout << "  - Scroll: Zoom in/out" << std::endl;
    std::cout << "  - R: Reset camera" << std::endl;
    std::cout << "  - 1: Toggle test cube" << std::endl;
    std::cout << "  - 3: Toggle ground" << std::endl;
    std::cout << "  - ESC: Exit" << std::endl << std::endl;

    // Main loop
    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate delta time
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        // Process input
        processInput(window, deltaTime);

        // Camera matrices
        glm::mat4 projection = glm::perspective(glm::radians(FoV), 
                                               (float)windowWidth / windowHeight, 
                                               zNear, zFar);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 viewProj = projection * view;

        // Render test objects
        ground.render(viewProj);
        testCube.render(viewProj);
        // FPS tracking
        frames++;
        fTime += deltaTime;
        if (fTime > 2.0f) {
            float fps = frames / fTime;
            frames = 0;
            fTime = 0;
            
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) 
                   << "Debug Test | FPS: " << fps 
                   << " | Pos: (" << (int)cameraPos.x << ", " << (int)cameraPos.y << ", " << (int)cameraPos.z << ")"
                   << " | Cube: " << (renderRobot ? "ON" : "OFF")
                   << " | Ground: " << (renderGround ? "ON" : "OFF");
            glfwSetWindowTitle(window, stream.str().c_str());
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (!glfwWindowShouldClose(window));

    // Cleanup
    testCube.cleanup();
    ground.cleanup();

    glfwTerminate();
    return 0;
}

// ====================
// INPUT PROCESSING
// ====================
void processInput(GLFWwindow* window, float deltaTime) {
    // Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * deltaTime * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * deltaTime * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos.y -= cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos.y += cameraSpeed * deltaTime;
    
    // Reset camera
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        cameraPos = glm::vec3(0.0f, 100.0f, 500.0f);
        yaw = -90.0f;
        pitch = -10.0f;
        
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
        
        std::cout << "Camera reset to: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
    }
    
    // Toggle test cube (using key 1)
    static bool key1Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (!key1Pressed) {
            renderRobot = !renderRobot;
            std::cout << "Test cube: " << (renderRobot ? "ON" : "OFF") << std::endl;
            key1Pressed = true;
        }
    } else {
        key1Pressed = false;
    }

    
    // Toggle ground (using key 3)
    static bool key3Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        if (!key3Pressed) {
            renderGround = !renderGround;
            std::cout << "Ground: " << (renderGround ? "ON" : "OFF") << std::endl;
            key3Pressed = true;
        }
    } else {
        key3Pressed = false;
    }
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    FoV -= (float)yoffset;
    if (FoV < 1.0f)
        FoV = 1.0f;
    if (FoV > 90.0f)
        FoV = 90.0f;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}