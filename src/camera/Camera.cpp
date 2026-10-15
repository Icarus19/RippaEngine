#include "Camera.h"

#include <glm/ext/matrix_transform.hpp>

Camera::Camera(
    glm::vec3 position,
    glm::vec3 up,
    float yaw,
    float pitch,
    float speed,
    float sensitivity,
    float zoom
)
    : Position(position),
      Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      Up(glm::vec3(0.0f)),
      Right(glm::vec3(0.0f)),
      WorldUp(up),
      Yaw(yaw),
      Pitch(pitch),
      Speed(speed),
      Sensitivity(sensitivity),
      Zoom(zoom)
{
    UpdateCameraVectors();
}

Camera::Camera(
    float posX,
    float posY,
    float posZ,
    float upX,
    float upY,
    float upZ,
    float yaw,
    float pitch
)
    : Camera(
		glm::vec3(posX, posY, posZ),
		glm::vec3(upX, upY, upZ),
		yaw,
		pitch
	)
{
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::vec3 Camera::GetPosition() const
{
    return Position;
}

// TODO: normalize movement so diagonal movement isn't faster
void Camera::ProcessMovement(CameraMovement direction, float deltaTime)
{
    float velocity = Speed * deltaTime;

    if (direction == CameraMovement::FORWARD)
        Position += Front * velocity;

    if (direction == CameraMovement::BACKWARD)
        Position -= Front * velocity;

    if (direction == CameraMovement::RIGHT)
        Position += Right * velocity;

    if (direction == CameraMovement::LEFT)
        Position -= Right * velocity;
}

void Camera::ProcessLook(float xOffset, float yOffset)
{
    xOffset *= Sensitivity;
    yOffset *= Sensitivity;

    Yaw += xOffset;
    Pitch += yOffset;

    Pitch = glm::clamp(Pitch, -89.0f, 89.0f);

    UpdateCameraVectors();
}

void Camera::UpdateCameraVectors()
{
    glm::vec3 front;

    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

    Front = glm::normalize(front);

    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}