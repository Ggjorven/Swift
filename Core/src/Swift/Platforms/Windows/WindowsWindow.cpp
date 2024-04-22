#include "swpch.h"
#include "WindowsWindow.hpp"

#include "Swift/Core/Application.hpp"
#include "Swift/Core/Events.hpp"
#include "Swift/Core/Logging.hpp"

#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

namespace Swift
{

	bool WindowsWindow::s_GLFWinitialized = false;
	uint32_t WindowsWindow::s_Instances = 0u;

	static void SetupAPIWindowHints();

	WindowsWindow::WindowsWindow(const WindowSpecification& properties)
	{
		m_Data.Name = properties.Name;
		m_Data.Width = properties.Width;
		m_Data.Height = properties.Height;
		m_Data.VSync = properties.VSync;

		if (!s_GLFWinitialized)
		{
			int succes = glfwInit();
			if (!succes)
				APP_LOG_ERROR("(GLFW) glfwInit() failed");

			s_GLFWinitialized = true;
			glfwSetErrorCallback(ErrorCallBack);
		}

		SetupAPIWindowHints();
		m_Window = glfwCreateWindow((int)properties.Width, (int)properties.Height, properties.Name.c_str(), nullptr, nullptr);
		s_Instances++;

		glfwSetWindowUserPointer(m_Window, &m_Data); //So we can access/get to the data in lambda functions
		if (properties.CustomPos) glfwSetWindowPos(m_Window, properties.X, properties.Y);

		//Event system
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event = WindowResizeEvent(width, height);
			data.CallBack(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event = WindowCloseEvent();
			data.CallBack(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyPressedEvent event = KeyPressedEvent(key, 0);
				data.CallBack(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event = KeyReleasedEvent(key);
				data.CallBack(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event = KeyPressedEvent(key, 1);
				data.CallBack(event);
				break;
			}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event = KeyTypedEvent(keycode);
			data.CallBack(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent event = MouseButtonPressedEvent(button);
				data.CallBack(event);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent event = MouseButtonReleasedEvent(button);
				data.CallBack(event);
				break;
			}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event = MouseScrolledEvent((float)xOffset, (float)yOffset);
			data.CallBack(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event = MouseMovedEvent((float)xPos, (float)yPos);
			data.CallBack(event);
		});
	}

	WindowsWindow::~WindowsWindow()
	{
		glfwDestroyWindow(m_Window);
		s_Instances--;

		if (s_Instances == 0)
			glfwTerminate();
	}

	void WindowsWindow::OnUpdate()
	{
		APP_PROFILE_SCOPE("PollEvents");
		glfwPollEvents();
	}

	void WindowsWindow::OnRender()
	{
		APP_MARK_FRAME;
	}

	uint32_t WindowsWindow::GetPositionX() const
	{
		int xPos = 0, yPos = 0;
		glfwGetWindowPos(m_Window, &xPos, &yPos);
		return (uint32_t)xPos;
	}

	uint32_t WindowsWindow::GetPositionY() const
	{
		int xPos = 0, yPos = 0;
		glfwGetWindowPos(m_Window, &xPos, &yPos);
		return (uint32_t)yPos;
	}

	uint32_t WindowsWindow::GetMonitorWidth() const
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		return mode->height;
	}

	uint32_t WindowsWindow::GetMonitorHeight() const
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		return mode->width;
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		m_Data.VSync = enabled;

		// Note(Jorben): Resize recreates swapchain with new specs aka VSync
		Renderer::OnResize(m_Data.Width, m_Data.Height);
	}

	void WindowsWindow::SetTitle(const std::string& title)
	{
		glfwSetWindowTitle(m_Window, title.c_str());
	}

	void WindowsWindow::ErrorCallBack(int errorCode, const char* description)
	{
		APP_LOG_ERROR("[GLFW]: ({0}), {1}", errorCode, description);
	}

	void SetupAPIWindowHints()
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			break;

		default:
			APP_LOG_ERROR("Invalid API selected.");
			break;
		}
	}

}