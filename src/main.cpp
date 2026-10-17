#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera/Camera.h"
#include "input/Input.h"
#include "renderer/Texture.h"
#include "renderer/Mesh.h"

// TODO: Create a Shader class that takes one shader file with both vertex and fragment

// Defining funtions
std::string LoadShaderSource(const char* path);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float SCR_NEAR = 0.1f;
const float SCR_FAR = 1000.0f;
const unsigned int SCR_FOV = 90;

bool firstMouse = true;
float lastX = SCR_WIDTH * 0.5;
float lastY = SCR_HEIGHT * 0.5;
float yaw = -90.0f;
float pitch = 0.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float imageSwap = 0.1f;

// Usually main has to be last, but because we declare our functions at the top we can define main first
int main()
{
    // Initialization
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Window creation using glfw
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "RippaEngine", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    Input input(window, camera);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetCursorPosCallback(window, mouse_callback);
    
    // Load opengl using glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Loading and compiling shaders
    std::string vertexSource = LoadShaderSource("./shaders/vertexshader.glsl");
    std::string fragmentSource = LoadShaderSource("./shaders/fragmentshader.glsl");
    const char* vertexSourcePointer = vertexSource.c_str();
    const char* fragmentSourcePointer = fragmentSource.c_str();
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSourcePointer, NULL);
    glCompileShader(vertexShader);
    
    // check vertex compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSourcePointer, NULL);
    glCompileShader(fragmentShader);
    
    // check fragment compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Create and attach shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // check linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Cleaning up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // TODO: Remove
    // TEMP: vertex data and indices this makes a square
    // We will use 3 attributes, position, normals and texcoord
    float pVertices[] = {
        -0.5f, -0.5f,  0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.5f, -0.5f,  0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
         0.5f,  0.5f,  0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
    };
    unsigned int pIndices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    // TEMP: this is just one triangle
    float tVertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
        0.0f, 0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.5f, 1.0f,
    };
    unsigned int tIndices[] = {
       0, 1, 2,
    };
    // TEMP: Box
    float cVertices[] = {
        // Position             Normal              TexCoord
        // Front
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
  
        // Back
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f, 1.0f,

        // Left
        -0.5f,  0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
  
        // Right
         0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,    1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,    0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,
 
        // Top
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,
    
        // Bottom
        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
    };
    unsigned int cIndices[] = {
        // Front
        0, 1, 2,        2, 3, 0,
        // Back 
        4, 7, 6,        6, 5, 4,
        // Left 
        8, 9, 10,       10, 11, 8,
        // Right    
        12, 13, 14,     14, 15, 12,
        // Top  
        16, 17, 18,     18, 19, 16,
        // Bottom   
        20, 21, 22,     22, 23, 20,
    };
    
    // Multiple object positions
    glm::vec3 oPositions[] = {
        glm::vec3( 0.0f, 0.0f, 0.0f),
        glm::vec3( 2.0f, 5.0f, -15.0f),
        glm::vec3( -1.5f, -2.2f, -2.5f),
        glm::vec3( -3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3( -1.7f, 3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f, 2.0f, -2.5f),
        glm::vec3( 1.5f, 0.2f, -1.5f),
        glm::vec3( -1.3f, 1.0f, -1.5f),
    };
    
    Mesh plane(pVertices, sizeof(pVertices), pIndices, std::size(pIndices));
    Mesh triangle(tVertices, sizeof(tVertices), tIndices, std::size(tIndices));
    Mesh cube(cVertices, sizeof(cVertices), cIndices, std::size(cIndices));
    Mesh* meshes[11] = { &cube, &plane, &cube, &triangle, &cube, &triangle, &cube, &plane, &cube, &triangle, &cube };
    std::mt19937 rng(std::random_device{}());
    std::ranges::shuffle(meshes, rng);
    
    Texture texture;
    texture.Bind(0);
    Texture texture16("./assets/default16bit.png");
    // Set second texture as uniform because the first one is automatic
    glUseProgram(shaderProgram); // Set shader before any uniforms are set
    // Static uniforms can be bound to a program before the loop
    // Changing program doesn't unbind any uniforms so we can change freely
    glUniform1i(glGetUniformLocation(shaderProgram, "texture0"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 1);
    
    // Matrices uniforms
    int modelUniform = glGetUniformLocation(shaderProgram, "model");
    int viewUniform = glGetUniformLocation(shaderProgram, "view");
    int projectionUniform = glGetUniformLocation(shaderProgram, "projection");
    
    int timeUniform = glGetUniformLocation(shaderProgram, "time");
    int zoomUniform = glGetUniformLocation(shaderProgram, "zoom");
    
    
    // Enable depth
    glEnable(GL_DEPTH_TEST);
    
    // Matrix spaces
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    
    // Projection
    glm::mat4 projection = glm::perspective(glm::radians((float)SCR_FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, SCR_NEAR, SCR_FAR);
    //glm::mat4 projection = glm::ortho(0.0f, SCR_WIDTH, 0.0f, SCR_HEIGHT, SCR_NEAR, SCR_FAR);
    
    // Static translations and rotations
    
    
    // Let there be light
    unsigned int lightVAO, lightVBO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cVertices), cVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Rendering pipeline
    while (!glfwWindowShouldClose(window))
    {
        // Frametime
        float time = glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;
        
        // input
        input.ProcessKeyboard(time);
        
        // render
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Reset matrices
        // Swap shader back to objects every frame
        model = glm::mat4(1.0f);
        view = glm::mat4(1.0f);
        
        // update matrix
        //model = glm::rotate(model, time * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        view = camera.GetViewMatrix();
        
        // update uniforms
        glUniformMatrix4fv(modelUniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewUniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform1f(timeUniform, time);
        glUniform1f(zoomUniform, imageSwap);
        
        // Draws
        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, oPositions[i]);
            float angle = 60.0f * i + time * 50.0f;
            
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            glUniformMatrix4fv(modelUniform, 1, GL_FALSE, glm::value_ptr(model));
            texture.Bind(0);
            meshes[i]->Draw();
        }
        // Switch shader to lightsource
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(5.0f, 5.0f, 5.0f));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    
    // de-allocate resources
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

std::string LoadShaderSource(const char* path)
{
    std::ifstream file(path);
    
    if (!file)
    {
        throw std::runtime_error("Failed to load shader source");
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return buffer.str();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}