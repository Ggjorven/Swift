#include "swpch.h"
#include "WindowsInput.hpp"

#include <GLFW/glfw3.h>

#include "Swift/Core/Application.hpp"

namespace Swift
{

    bool WindowsInput::IsKeyPressedImplementation(Key keycode)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        int state = glfwGetKey(window, (int)keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool WindowsInput::IsMousePressedImplementation(MouseButton button)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        int state = glfwGetMouseButton(window, (int)button);
        return state == GLFW_PRESS;
    }

    glm::vec2 WindowsInput::GetMousePositionImplementation()
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);

        return { xPos, yPos };
    }

    void WindowsInput::SetCursorPositionImplementation(glm::vec2 position)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        glfwSetCursorPos(window, position.x, position.y);
    }

    void WindowsInput::SetCursorModeImplementation(CursorMode mode)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        glfwSetInputMode(window, GLFW_CURSOR, (int)mode);
    }

}
