#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

enum class CameraMovement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera
{
public:
    Camera(
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = 0.0f,
        float speed = 2.5f,
        float sensitivity = 0.1f,
        float zoom = 90.0f
    );

    Camera(
        float posX,
        float posY,
        float posZ,
        float upX,
        float upY,
        float upZ,
        float yaw,
        float pitch
    );

    glm::mat4 GetViewMatrix() const;
    glm::vec3 GetPosition() const;

    void ProcessMovement(CameraMovement direction, float deltaTime);
    void ProcessLook(float xOffset, float yOffset);

private:
    void UpdateCameraVectors();

    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float Speed;
    float Sensitivity;
    float Zoom;
};