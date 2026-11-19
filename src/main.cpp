#include <algorithm>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <assets/ShaderLibrary.h>

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

#include "renderer/BVH.h"
#include "renderer/Material.h"
#include "assets/MaterialLoader.h"

// TODO: Create a Shader class that takes one shader file with both vertex and fragment

// Defining funtions
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void appendMesh(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        uint32_t materialIndex,
        const glm::mat4& model
);
static std::vector<GPUVertex> gpuVertices;
static std::vector<unsigned int> gpuIndices;
static std::vector<uint32_t> gpuTriMaterials;

void printBuf(const char* tag, GLuint id) {
    GLint size = -1;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &size);
    std::cout << tag << ": id " << id
              << " isBuffer=" << (int)glIsBuffer(id)
              << " size=" << size << "\n";
}

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float SCR_NEAR = 0.1f;
const float SCR_FAR = 10000.0f;
const unsigned int SCR_FOV = 90;

bool firstMouse = true;
float lastX = SCR_WIDTH * 0.5;
float lastY = SCR_HEIGHT * 0.5;
float yaw = -90.0f;
float pitch = 0.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float imageSwap = 0.1f;
unsigned int debugDepth = 0; // TODO: make interface to change
static unsigned int Frames = 0;

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
    
    std::vector<Mesh> meshes;
    std::string path = "./assets/models/sponza/sponza.obj";
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals);
    
    if (!scene)
    {
        std::cerr << "Assimp failed: "
        << importer.GetErrorString() << std::endl;
        return 0;
    }
    
    
    std::filesystem::path modelDir = std::filesystem::path(path).parent_path();
    std::vector<Material> materials(scene->mNumMaterials);
    
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        const aiMaterial* m = scene->mMaterials[i];
        MaterialLoader::LoadSlot(modelDir, m, aiTextureType_DIFFUSE, TextureSlot::Albedo, ColorSpace::SRGB, materials[i]);
        MaterialLoader::LoadSlot(modelDir, m, aiTextureType_HEIGHT, TextureSlot::Normal, ColorSpace::Linear, materials[i]);
        MaterialLoader::LoadSlot(modelDir, m, aiTextureType_SPECULAR, TextureSlot::Specular, ColorSpace::Linear, materials[i]);
        MaterialLoader::LoadSlot(modelDir, m, aiTextureType_OPACITY, TextureSlot::Alpha, ColorSpace::Linear, materials[i]);
        
        aiColor3D c;
        if(m->Get(AI_MATKEY_COLOR_DIFFUSE, c) == AI_SUCCESS) materials[i].albedo = { c.r, c.g, c.b };
        if(m->Get(AI_MATKEY_COLOR_EMISSIVE, c) == AI_SUCCESS) materials[i].emission = { c.r, c.g, c.b };
    }
    
    std::vector<GPUMaterial> gpuMats;
    for (const Material& m : materials)
    {
        auto handle = [&](TextureSlot s) -> uint64_t {
            const Texture* t = m.GetTexture(s);
            return t ? t->GetHandle() : 0;
        };
        gpuMats.push_back({ m.albedo, m.roughness, m.emission, m.metallic,
                        handle(TextureSlot::Albedo), handle(TextureSlot::Normal),
                        handle(TextureSlot::Specular), handle(TextureSlot::Alpha) });
    }
    
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        
        std::cout << "Vertex count: "
              << mesh->mNumVertices
              << '\n'
              << std::flush;

        std::cout << "Face count: "
                  << mesh->mNumFaces
                  << '\n'
                  << std::flush;
        
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        vertices.reserve(mesh->mNumVertices);
        indices.reserve(mesh->mNumFaces * 3);
        
        std::cout
            << "Loading mesh " << i + 1
            << "/" << scene->mNumMeshes
            << ": "
            << mesh->mNumVertices
            << " vertices, "
            << mesh->mNumFaces
            << " faces\n"
            << std::flush;
        
        for (unsigned int j = 0; j < mesh->mNumVertices; j++)
        {
            Vertex vertex;
        
            vertex.position = {
                mesh->mVertices[j].x,
                mesh->mVertices[j].y,
                mesh->mVertices[j].z
            };
        
            if (mesh->HasNormals())
            {
                vertex.normal = {
                    mesh->mNormals[j].x,
                    mesh->mNormals[j].y,
                    mesh->mNormals[j].z
                };   
            } else {
                vertex.normal = { 0.0f, 0.0f, 0.0f };
            }
        
            if (mesh->HasTextureCoords(0))
            {
                vertex.texCoord = {
                    mesh->mTextureCoords[0][j].x,
                    mesh->mTextureCoords[0][j].y
                };   
            } else {
                vertex.texCoord = { 0.0f, 0.0f };
            }
        
            vertices.push_back(vertex);
        }
    
        for (unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            const aiFace& face = mesh->mFaces[j];
        
            for (unsigned int k = 0; k < face.mNumIndices; k++)
            {
                indices.push_back(face.mIndices[k]);
            }
        }
    
        meshes.emplace_back(std::move(vertices), std::move(indices), mesh->mMaterialIndex);
    }
    

    unsigned int shaderProgram = glCreateProgram();
    ShaderLibrary::Init("./shaders/helpers/common.glsl");
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
    
    /*Mesh plane(pVertices, sizeof(pVertices), pIndices, std::size(pIndices));
    Mesh triangle(tVertices, sizeof(tVertices), tIndices, std::size(tIndices));
    Mesh cube(cVertices, sizeof(cVertices), cIndices, std::size(cIndices));
    Mesh* meshes[11] = { &cube, &plane, &cube, &triangle, &cube, &triangle, &cube, &plane, &cube, &triangle, &cube };
    std::mt19937 rng(std::random_device{}());
    std::ranges::shuffle(meshes, rng);
    */
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
        
        .intensity = 12.0f,
        .cutoff = glm::cos(glm::radians(18.0f)), // TODO: make gradual
        .outerCutoff = glm::cos(glm::radians(75.0f)),
        .constant = 1.0f,
        .linear = 0.0009f,
        .quadratic = 0.00032f
    };
    Light light1{
        .position = glm::vec3(2.5f, 5.0f, 2.5f),
        .direction = glm::vec3(1.0f, 0.0f, 0.0f),
        .ambient = glm::vec3(0.005f, 0.005f, 0.005f),
        .diffuse = glm::vec3(1.0f, 0.98f, 0.96f),
        .specular = glm::vec3(1.0f, 1.0f, 1.0f),
        
        .intensity = 6.0f,
        .cutoff = glm::cos(glm::radians(12.5f)), // TODO: make gradual
        .outerCutoff = glm::cos(glm::radians(25.0f)),
        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f
    };
    
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
    texture.Bind(0);
    int materialSpecularUniform = glGetUniformLocation(shaderProgram, "material.specular");
    glUniform3fv(materialSpecularUniform, 1, glm::value_ptr(glm::vec3(0.5f)));
    int materialShininessUniform = glGetUniformLocation(shaderProgram, "material.shininess");
    glUniform1f(materialShininessUniform, 32.0f);
    
    unsigned int raytraceProgram = glCreateProgram();
    Shader raytraceVertexShader("./shaders/raytracevertexshader.glsl", GL_VERTEX_SHADER);
    Shader raytraceFragmentShader("./shaders/raytracefragmentshader.glsl", GL_FRAGMENT_SHADER);
    glAttachShader(raytraceProgram, raytraceVertexShader.GetId());
    glAttachShader(raytraceProgram, raytraceFragmentShader.GetId());
    glLinkProgram(raytraceProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glUseProgram(raytraceProgram);
    
    int raytraceProjectionUniform = glGetUniformLocation(raytraceProgram, "inverseProjection");
    int raytraceViewUniform = glGetUniformLocation(raytraceProgram, "inverseView");
    
    // Matrix spaces
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    
    // Projection
    glm::mat4 projection = glm::perspective(glm::radians((float)SCR_FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, SCR_NEAR, SCR_FAR);
    //glm::mat4 projection = glm::ortho(0.0f, SCR_WIDTH, 0.0f, SCR_HEIGHT, SCR_NEAR, SCR_FAR);
    
    // Mesh for ray tracing
    Mesh raytraceMesh = Mesh::CreatePlane();
    for (const Mesh& mesh : meshes)
    {
        appendMesh(mesh.GetVertices(), mesh.GetIndices(), mesh.GetMaterialIndex(), model);
    }
    
    std::cout << std::endl;
    // Optimize raytrace with BVH
    BVH bvh(gpuVertices, gpuIndices, gpuTriMaterials);
    
    unsigned int vertexSSBO, indexSSBO, nodeSSBO;
    glGenBuffers(1, &vertexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        gpuVertices.size() * sizeof(GPUVertex),
        gpuVertices.data(),
        GL_STATIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexSSBO);
    
    glGenBuffers(1, &indexSSBO); // Use bvh instead of our old gpuIndices
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        bvh.GetIndices().size() * sizeof(unsigned int),
        bvh.GetIndices().data(),
        GL_STATIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indexSSBO);
    
    glGenBuffers(1, &nodeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        bvh.GetNodes().size() * sizeof(BVHNode),
        bvh.GetNodes().data(),
        GL_STATIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, nodeSSBO);
    
    // Material buffers
    unsigned int triMatSSBO, matSSBO;
    glGenBuffers(1, &triMatSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triMatSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        bvh.GetTriMaterials().size() * sizeof(uint32_t),
        bvh.GetTriMaterials().data(),
        GL_STATIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, triMatSSBO);
    
    glGenBuffers(1, & matSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, matSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        gpuMats.size() * sizeof(GPUMaterial),
        gpuMats.data(),
        GL_STATIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, matSSBO);
    
    // Get wiremesh for bvh debugging
    unsigned int wireMeshProgram = glCreateProgram();
    Shader BVHvertexShader("./shaders/bvhvertexshader.glsl", GL_VERTEX_SHADER);
    Shader BVHfragmentShader("./shaders/bvhfragmentshader.glsl", GL_FRAGMENT_SHADER);
	
    glAttachShader(wireMeshProgram, BVHvertexShader.GetId());
    glAttachShader(wireMeshProgram, BVHfragmentShader.GetId());
    glLinkProgram(wireMeshProgram);
    glGetProgramiv(wireMeshProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(wireMeshProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glUseProgram(wireMeshProgram);
    
    unsigned int wireMeshVAO, wireMeshVBO;
    
    glGenVertexArrays(1, &wireMeshVAO);
    glGenBuffers(1, &wireMeshVBO);
    
    glBindVertexArray(wireMeshVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wireMeshVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    
    
    // TODO: Can be repeated at runtime when depth changes
    std::vector<glm::vec3> wireMesh = bvh.GetBoxLines(debugDepth);
    glBindBuffer(GL_ARRAY_BUFFER, wireMeshVBO);
    glBufferData(GL_ARRAY_BUFFER, wireMesh.size() * sizeof(glm::vec3), wireMesh.data(), GL_DYNAMIC_DRAW);
    int wireMeshCount = (GLsizei)(wireMesh.size());
    
    int BVHViewProjectionUniform = glGetUniformLocation(wireMeshProgram, "viewProjection");
    int BVHTimeUniform = glGetUniformLocation(raytraceProgram, "time");
    //glUniform1i(glGetUniformLocation(wireMeshProgram, "albedoMap"), 0);
    
    // Static translations and rotations
    
    
    // Rendering pipeline
    while (!glfwWindowShouldClose(window))
    {
        // Frametime
        float time = glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;
        Frames++;
        
        glUseProgram(raytraceProgram);
        
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
        /*for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, oPositions[i]);
            float angle = 60.0f * i + time * 50.0f;
            
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            glUniformMatrix4fv(modelUniform, 1, GL_FALSE, glm::value_ptr(model));
            //texture.Bind(0);
            meshes[i]->Draw();
        }*/
        /*model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(5.0f, 5.0f, 5.0f));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw();
        */
        // Enable depth
        glEnable(GL_DEPTH_TEST);
        glUniformMatrix4fv(raytraceProjectionUniform, 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
        glUniformMatrix4fv(raytraceViewUniform, 1, GL_FALSE, glm::value_ptr(glm::inverse(view)));
        glUniform1f(BVHTimeUniform, time);
        raytraceMesh.Draw();
        
        // Draw Debug
        if (debugDepth > 0)
        {
            glUseProgram(wireMeshProgram);
            glUniformMatrix4fv(BVHViewProjectionUniform, 1, GL_FALSE, glm::value_ptr(projection * view));
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(wireMeshVAO);
            glDrawArrays(GL_LINES, 0, wireMeshCount);
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    std::cout << "Average fps: " << Frames / glfwGetTime() << std::endl;
    
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

void appendMesh(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        uint32_t materialIndex,
        const glm::mat4& model
    )
{
    unsigned int base = (unsigned int)gpuVertices.size();
    glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
    
    for (const Vertex& v : vertices)
    {
        GPUVertex g;
        g.position = model * glm::vec4(v.position, 1.0f);
        g.normal = glm::vec4(glm::normalize(normalMat * v.normal), 1.0f);
        g.texCoord = glm::vec4(v.texCoord, 0.0f, 0.0f);
        gpuVertices.push_back(g);
    }
    
    for (unsigned int i : indices)
        gpuIndices.push_back(base + i);
    
    gpuTriMaterials.insert(gpuTriMaterials.end(), indices.size() / 3, materialIndex);
}