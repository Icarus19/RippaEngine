#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <renderer/Shader.h>

#include "camera/Camera.h"
#include "input/Input.h"
#include "renderer/Texture.h"
#include "renderer/Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// TODO: Create a Shader class that takes one shader file with both vertex and fragment

// Defining funtions
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

// TODO: radius with falloff
// TODO: also, light class with different kinds
struct Light
{
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    
    float intensity;
    float cutoff;
    float outerCutoff;
    float constant;
    float linear;
    float quadratic;
};

struct Material
{
    Texture &diffuse;
    glm::vec3 specular;
    float shininess;
};

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
    
    // Load opengl using glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("./assets/models/bunny.obj", aiProcess_Triangulate | aiProcess_GenNormals);
    
    if (!scene)
    {
        std::cerr << "Assimp failed: "
        << importer.GetErrorString() << std::endl;
    }
    std::cout << "Import succeeded!\n";
    std::cout << "Meshes: " << scene->mNumMeshes << '\n';
    std::cout << "Materials: " << scene->mNumMaterials << '\n';
    std::cout << "Root node: "
              << scene->mRootNode->mName.C_Str() << '\n';

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];

        std::cout << "\nMesh " << i << '\n';
        std::cout << "  Name: " << mesh->mName.C_Str() << '\n';
        std::cout << "  Vertices: " << mesh->mNumVertices << '\n';
        std::cout << "  Faces: " << mesh->mNumFaces << '\n';
        std::cout << "  Has normals: "
                  << (mesh->HasNormals() ? "yes" : "no") << '\n';
        std::cout << "  Has texture coordinates: "
                  << (mesh->HasTextureCoords(0) ? "yes" : "no") << '\n';

        if (mesh->mNumVertices > 0)
        {
            const aiVector3D& v = mesh->mVertices[0];

            std::cout << "  First vertex: ("
                      << v.x << ", "
                      << v.y << ", "
                      << v.z << ")\n";
        }

        if (mesh->mNumFaces > 0)
        {
            const aiFace& face = mesh->mFaces[0];

            std::cout << "  First face indices:";

            for (unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                std::cout << ' ' << face.mIndices[j];
            }

            std::cout << '\n';
        }
    }

    unsigned int shaderProgram = glCreateProgram();
    Shader vertexShader("./shaders/vertexshader.glsl", GL_VERTEX_SHADER);
    Shader fragmentShader("./shaders/fragmentshader.glsl", GL_FRAGMENT_SHADER);
	
    glAttachShader(shaderProgram, vertexShader.GetId());
    glAttachShader(shaderProgram, fragmentShader.GetId());
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

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
    
    //Texture texture;
    //texture.Bind(0);
    Texture texture("./assets/default16bit.png");
    // Set second texture as uniform because the first one is automatic
    glUseProgram(shaderProgram); // Set shader before any uniforms are set
    // Static uniforms can be bound to a program before the loop
    // Changing program doesn't unbind any uniforms so we can change freely
    //glUniform1i(glGetUniformLocation(shaderProgram, "texture0"), 0);
    //glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 1);
    
    // Let there be light
    Light light{
        .position = camera.GetPosition(), // lets use camera position for light
        .direction = camera.GetDirection(),
        .ambient = glm::vec3(0.05f, 0.05f, 0.05f),
        .diffuse = glm::vec3(1.0f, 0.98f, 0.96f),
        .specular = glm::vec3(1.0f, 1.0f, 1.0f),
        
        .intensity = 0.5f,
        .cutoff = glm::cos(glm::radians(12.5f)), // TODO: make gradual
        .outerCutoff = glm::cos(glm::radians(25.0f)),
        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f
    };
    Light light1{
        .position = glm::vec3(0.0f, 0.0f, 0.0f),
        .direction = glm::vec3(1.0f, 0.0f, 0.0f),
        .ambient = glm::vec3(0.05f, 0.05f, 0.05f),
        .diffuse = glm::vec3(1.0f, 0.98f, 0.96f),
        .specular = glm::vec3(1.0f, 1.0f, 1.0f),
        
        .intensity = 6.0f,
        .cutoff = glm::cos(glm::radians(12.5f)), // TODO: make gradual
        .outerCutoff = glm::cos(glm::radians(25.0f)),
        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f
    };
    Material material(
        texture, 
        glm::vec3(0.5f, 0.5f, 0.5f), 
        32.0f
    );
    
    // Matrices uniforms
    int modelUniform = glGetUniformLocation(shaderProgram, "model");
    int viewUniform = glGetUniformLocation(shaderProgram, "view");
    int projectionUniform = glGetUniformLocation(shaderProgram, "projection");
    int cameraUniform = glGetUniformLocation(shaderProgram, "viewPosition");
    
    int timeUniform = glGetUniformLocation(shaderProgram, "time");
    int zoomUniform = glGetUniformLocation(shaderProgram, "zoom");
    
    // Light
    int lightPositionUniform = glGetUniformLocation(shaderProgram, "lights[0].position");
    glUniform3fv(lightPositionUniform, 1, glm::value_ptr(light.position));
    int lightDirectionUniform = glGetUniformLocation(shaderProgram, "lights[0].direction");
    glUniform3fv(lightDirectionUniform, 1, glm::value_ptr(light.direction));
    int lightAmbientUniform = glGetUniformLocation(shaderProgram, "lights[0].ambient");
    glUniform3fv(lightAmbientUniform, 1, glm::value_ptr(light.ambient));
    int lightDiffuseUniform = glGetUniformLocation(shaderProgram, "lights[0].diffuse");
    glUniform3fv(lightDiffuseUniform, 1, glm::value_ptr(light.diffuse));
    int lightSpecularUniform = glGetUniformLocation(shaderProgram, "lights[0].specular");
    glUniform3fv(lightSpecularUniform, 1, glm::value_ptr(light.specular));
    
    int lightIntensityUniform = glGetUniformLocation(shaderProgram, "lights[0].intensity");
    glUniform1f(lightIntensityUniform, light.intensity);
    int lightCutoffUniform = glGetUniformLocation(shaderProgram, "lights[0].cutoff");
    glUniform1f(lightCutoffUniform, light.cutoff);
    int lightOuterCutoffUniform = glGetUniformLocation(shaderProgram, "lights[0].outerCutoff");
    glUniform1f(lightOuterCutoffUniform, light.outerCutoff);
    int lightConstantUniform = glGetUniformLocation(shaderProgram, "lights[0].constant");
    glUniform1f(lightConstantUniform, light.constant);
    int lightLinearUniform = glGetUniformLocation(shaderProgram, "lights[0].linear");
    glUniform1f(lightLinearUniform, light.linear);
    int lightQuadraticUniform = glGetUniformLocation(shaderProgram, "lights[0].quadratic");
    glUniform1f(lightQuadraticUniform, light.quadratic);
    
    // light1
    int light1PositionUniform = glGetUniformLocation(shaderProgram, "lights[1].position");
    glUniform3fv(light1PositionUniform, 1, glm::value_ptr(light1.position));
    int light1DirectionUniform = glGetUniformLocation(shaderProgram, "lights[1].direction");
    glUniform3fv(light1DirectionUniform, 1, glm::value_ptr(light1.direction));
    int light1AmbientUniform = glGetUniformLocation(shaderProgram, "lights[1].ambient");
    glUniform3fv(light1AmbientUniform, 1, glm::value_ptr(light1.ambient));
    int light1DiffuseUniform = glGetUniformLocation(shaderProgram, "lights[1].diffuse");
    glUniform3fv(light1DiffuseUniform, 1, glm::value_ptr(light1.diffuse));
    int light1SpecularUniform = glGetUniformLocation(shaderProgram, "lights[1].specular");
    glUniform3fv(light1SpecularUniform, 1, glm::value_ptr(light1.specular));
    
    int light1IntensityUniform = glGetUniformLocation(shaderProgram, "lights[1].intensity");
    glUniform1f(light1IntensityUniform, light1.intensity);
    int light1CutoffUniform = glGetUniformLocation(shaderProgram, "lights[1].cutoff");
    glUniform1f(light1CutoffUniform, light1.cutoff);
    int light1OuterCutoffUniform = glGetUniformLocation(shaderProgram, "lights[1].outerCutoff");
    glUniform1f(light1OuterCutoffUniform, light1.outerCutoff);
    int light1ConstantUniform = glGetUniformLocation(shaderProgram, "lights[1].constant");
    glUniform1f(light1ConstantUniform, light1.constant);
    int light1LinearUniform = glGetUniformLocation(shaderProgram, "lights[1].linear");
    glUniform1f(light1LinearUniform, light1.linear);
    int light1QuadraticUniform = glGetUniformLocation(shaderProgram, "lights[1].quadratic");
    glUniform1f(light1QuadraticUniform, light1.quadratic);
    
    // Material
    int materialDiffuseUniform = glGetUniformLocation(shaderProgram, "material.diffuse");
    glUniform1i(materialDiffuseUniform, 0);
    material.diffuse.Bind(0);
    int materialSpecularUniform = glGetUniformLocation(shaderProgram, "material.specular");
    glUniform3fv(materialSpecularUniform, 1, glm::value_ptr(material.specular));
    int materialShininessUniform = glGetUniformLocation(shaderProgram, "material.shininess");
    glUniform1f(materialShininessUniform, material.shininess);
    
    
    // Enable depth
    glEnable(GL_DEPTH_TEST);
    
    // Matrix spaces
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    
    // Projection
    glm::mat4 projection = glm::perspective(glm::radians((float)SCR_FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, SCR_NEAR, SCR_FAR);
    //glm::mat4 projection = glm::ortho(0.0f, SCR_WIDTH, 0.0f, SCR_HEIGHT, SCR_NEAR, SCR_FAR);
    
    // Static translations and rotations
    
    
    // Rendering pipeline
    while (!glfwWindowShouldClose(window))
    {
        // Frametime
        float time = glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;
        
        // input
        input.ProcessKeyboard(deltaTime);
        
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
        light.position = camera.GetPosition();
        glUniform3fv(lightPositionUniform, 1, glm::value_ptr(light.position));
        light.direction = camera.GetDirection();
        glUniform3fv(lightDirectionUniform, 1, glm::value_ptr(light.direction));
        
        // update uniforms
        glUniformMatrix4fv(modelUniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewUniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(cameraUniform, 1, glm::value_ptr(camera.GetPosition()));
        
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
            //texture.Bind(0);
            meshes[i]->Draw();
        }
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}