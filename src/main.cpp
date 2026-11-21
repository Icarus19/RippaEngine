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
#include "renderer/Program.h"
#include "renderer/Raytracer.h"

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

constexpr int NUM_CASCADES = 1;
constexpr float L0 = 0.5f;
bool cascadesDirty = true;
    

// TODO: Make interface for realtime diagnostics
static unsigned int Frames = 0;
static unsigned int raytraceDispatch = 0;
static unsigned int cascadeDispatch = 0;

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
    Shader vertexShader("./shaders/vertex/vertexshader.glsl", GL_VERTEX_SHADER);
    Shader fragmentShader("./shaders/fragments/fragmentshader.glsl", GL_FRAGMENT_SHADER);
	
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
    
    Program displayProgram;
    Shader displayVertexShader("./shaders/vertex/raytracevertexshader.glsl", GL_VERTEX_SHADER);
    Shader displayFragmentShader("./shaders/fragments/raytracefragmentshader.glsl", GL_FRAGMENT_SHADER);
    displayProgram.AttachShader(displayVertexShader);
    displayProgram.AttachShader(displayFragmentShader);
    displayProgram.Link();
    
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
    
    Program raytraceProgram;
    Shader raytraceShader("./shaders/computes/raytrace.glsl", GL_COMPUTE_SHADER);
    raytraceProgram.AttachShader(raytraceShader);
    raytraceProgram.Link();
    
    
    //glUniform1i(glGetUniformLocation(wireMeshProgram, "albedoMap"), 0);
    // Generate new texture for output
    unsigned int displayTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &displayTexture);
    glTextureStorage2D(displayTexture, 1, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT);
    glTextureParameteri(displayTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(displayTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindImageTexture(0, displayTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    displayProgram.Use();
    glBindTextureUnit(0, displayTexture);
    
    // Buffer declarations, note: buffers are shared between all programs
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
    
    
    // Radiant cascades setup
    std::vector<Cascade> cascades(NUM_CASCADES);
    
    glm::vec3 bmin = bvh.GetNodes()[0].boundsMin - glm::vec3(L0);
    glm::vec3 bmax = bvh.GetNodes()[0].boundsMax - glm::vec3(L0);
    glm::vec3 gridOrigin = bmin;
    
    glm::ivec3 p0 = glm::ivec3(glm::ceil((bmax - bmin) / L0));
    int div = 1 << (NUM_CASCADES - 1);
    p0 = ((p0 + div - 1) / div) * div;
    
    for (int i = 0; i < NUM_CASCADES; i++)
    {
        Cascade& c = cascades[i];
        c.tile = 4 << i;
        c.spacing = L0 * (1 << i);
        c.tStart = (i == 0) ? 0.0f : L0 * std::pow(4.0f, i - 1);
        c.tEnd = (i == NUM_CASCADES - 1) ? 1e4f : L0 * std::pow(4.0f, i);
        c.probes = p0 / (1 << i);
        c.size = { c.probes.x * c.tile, c.probes.y * c.tile, c.probes.z };
        
        glCreateTextures(GL_TEXTURE_3D, 1, &c.texture);
        glTextureStorage3D(c.texture, 1, GL_RGBA16F, c.size.x, c.size.y, c.size.z);
        glTextureParameteri(c.texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(c.texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    raytraceProgram.Use();
    RaytraceUniforms raytraceUniforms;
    raytraceUniforms.Query(raytraceProgram);
    glUniform1i (raytraceUniforms.cascade0, 2);
    glUniform3fv(raytraceUniforms.gridOrigin, 1, glm::value_ptr(gridOrigin));
    glUniform1f (raytraceUniforms.spacing0, cascades[0].spacing);
    glUniform3iv(raytraceUniforms.probeCount0, 1, glm::value_ptr(cascades[0].probes));
    glUniform1i (raytraceUniforms.tile0, cascades[0].tile);
    
    
    // Radiance Cascades Compute shader
    Program cascadeProgram;
    Shader cascadeShader("./shaders/computes/cascade.glsl", GL_COMPUTE_SHADER);
    cascadeProgram.AttachShader(cascadeShader);
    cascadeProgram.Link();
    
    CascadeUniforms cascadeUniforms;
    cascadeUniforms.Query(cascadeProgram);
    
    
    // Get wiremesh for bvh debugging
    unsigned int wireMeshProgram = glCreateProgram();
    Shader BVHvertexShader("./shaders/vertex/bvhvertexshader.glsl", GL_VERTEX_SHADER);
    Shader BVHfragmentShader("./shaders/fragments/bvhfragmentshader.glsl", GL_FRAGMENT_SHADER);
	
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
    
    std::vector<glm::vec3> wireMesh = bvh.GetBoxLines(debugDepth);
    glBindBuffer(GL_ARRAY_BUFFER, wireMeshVBO);
    glBufferData(GL_ARRAY_BUFFER, wireMesh.size() * sizeof(glm::vec3), wireMesh.data(), GL_DYNAMIC_DRAW);
    int wireMeshCount = (GLsizei)(wireMesh.size());
    
    int BVHViewProjectionUniform = glGetUniformLocation(wireMeshProgram, "viewProjection");
    
    
    // Static translations and rotations
    unsigned int debugQuery[2];
    glGenQueries(2, debugQuery);
    // 0 = cascade dispatch
    // 1 = Ray trace dispatch
    
    // Rendering pipeline
    while (!glfwWindowShouldClose(window))
    {
        // Frametime
        float time = glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;
        Frames++;
        
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
        
        
        // TODO: Make passes
        // 1. Cascade Pass
        cascadeProgram.Use();
        if (cascadesDirty)
        {
            glBeginQuery(GL_TIME_ELAPSED, debugQuery[0]);
            Raytracer::UpdateCascades(cascadeProgram, cascadeUniforms, cascades, gridOrigin);
            glEndQuery(GL_TIME_ELAPSED);
            cascadeDispatch += debugQuery[0];
            cascadesDirty = false;
        }
        
        // 2. Camera Pass
        raytraceProgram.Use();
        glUniformMatrix4fv(raytraceUniforms.inverseProjection, 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
        glUniformMatrix4fv(raytraceUniforms.inverseView, 1, GL_FALSE, glm::value_ptr(glm::inverse(view)));
        glBindImageTexture(0, displayTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glBindTextureUnit(2, cascades[0].texture);
        
        glBeginQuery(GL_TIME_ELAPSED, debugQuery[1]);
        glDispatchCompute((SCR_WIDTH + 7) / 8, (SCR_HEIGHT + 7) / 8, 1);
        glEndQuery(GL_TIME_ELAPSED);
        raytraceDispatch += debugQuery[1];
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        
        // 3. Display
        displayProgram.Use();
        glEnable(GL_DEPTH_TEST);
        glBindTextureUnit(0, displayTexture);
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
    
    // Statistics
    std::cout   << "Average fps: " << Frames / glfwGetTime()
                << "\nAverage raytrace compute: " << raytraceDispatch / glfwGetTime()
                << "\nAverage cascade compute: " << cascadeDispatch / glfwGetTime()
                << std::endl;
    
    glfwDestroyWindow(window);
    
    // de-allocate resources
    glDeleteProgram(shaderProgram);
    glDeleteProgram(displayProgram.GetId());
    glDeleteProgram(wireMeshProgram);
    glDeleteProgram(cascadeProgram.GetId());
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

    /*
    glUseProgram(mergeProgram);
    glUniform3fv(glGetUniformLocation(mergeProgram, "gridOrigin"), 1, glm::value_ptr(gridOrigin));
    
    for (int i = 0; i < NUM_CASCADES; i++)
    {
        const Cascade& c = cascades[i];
        glUniform1f(glGetUniformLocation(mergeProgram, "spacing"), c.spacing);
        glUniform1i(glGetUniformLocation(mergeProgram, "tile"), c.tile);
        glUniform3iv(glGetUniformLocation(mergeProgram, "upperProbeCount"), 1, glm::value_ptr(cascades[i+1].probes));

        glBindImageTexture(0, c.texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindTextureUnit(1, cascades[i+1].texture);                 
        glUniform1i(glGetUniformLocation(mergeProg, "upperTex"), 1);
        glDispatchCompute((c.size.x + 7) / 8, (c.size.y + 7) / 8, c.size.z);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }*/
