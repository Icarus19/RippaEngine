#include "Input.h"
#include "camera/Camera.h"

Input::Input(GLFWwindow* window, Camera& camera)
    : m_window(window),
    m_camera(camera)
{
    glfwSetWindowUserPointer(m_window, this);
    glfwSetCursorPosCallback(m_window, &Input::CursorPositionCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Input::ProcessKeyboard(float deltaTime)
{
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera.ProcessMovement(CameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera.ProcessMovement(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera.ProcessMovement(CameraMovement::LEFT, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera.ProcessMovement(CameraMovement::RIGHT, deltaTime);
}

void Input::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (input != nullptr)
        input->OnCursorPositionCallback(xpos, ypos);
}

void Input::OnCursorPositionCallback(double xpos, double ypos)
{
    if (m_firstMouse)
    {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
        return;
    }
    
    const double xOffset = xpos - m_lastX;
    const double yOffset = m_lastY - ypos;
    
    m_lastX = xpos;
    m_lastY = ypos;
    
    m_camera.ProcessLook(
        static_cast<float>(xOffset),
        static_cast<float>(yOffset)    
    );
}
