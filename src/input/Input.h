#pragma once
#include <GLFW/glfw3.h>

class Camera;

class Input
{
public:
    Input(GLFWwindow* window, Camera& camera);
    
    void ProcessKeyboard(float deltaTime);
    
private:
    static void CursorPositionCallback(
        GLFWwindow* window,
        double xpos,
        double ypos
    );
    
    void OnCursorPositionCallback(double xpos, double ypos);
    
    GLFWwindow* m_window;
    Camera& m_camera;
    
    bool m_firstMouse = true;
    double m_lastX = 0.0;
    double m_lastY = 0.0;
};
