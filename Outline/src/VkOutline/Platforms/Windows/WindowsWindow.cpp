#include "vkpch.h"
#include "WindowsWindow.hpp"

#include "VkOutline/Core/Application.hpp"
#include "VkOutline/Core/Events.hpp"
#include "VkOutline/Core/Logging.hpp"

#include "VkOutline/Utils/Profiler.hpp"

//#include "VkOutline/Renderer/Renderer.hpp"

namespace VkOutline
{

	bool WindowsWindow::s_GLFWinitialized = false;
	uint32_t WindowsWindow::s_Instances = 0u;

	static void SetupAPIWindowHints();

	WindowsWindow::WindowsWindow()
	{
	}

	WindowsWindow::~WindowsWindow()
	{
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
		m_Data.Vsync = enabled;
	}

	void WindowsWindow::SetTitle(const std::string& title)
	{
		glfwSetWindowTitle(m_Window, title.c_str());
	}
	
	bool WindowsWindow::Init(const WindowSpecification& properties)
	{
		if (!s_GLFWinitialized)
		{
			int succes = glfwInit();
			if (!succes)
			{
				LV_LOG_ERROR("glfwInit() failed");
				return true;
			}
			s_GLFWinitialized = true;
			glfwSetErrorCallback(ErrorCallBack);
		}
		m_Data.Vsync = properties.VSync;

		SetupAPIWindowHints();
		m_Window = glfwCreateWindow((int)properties.Width, (int)properties.Height, properties.Name.c_str(), nullptr, nullptr);
		s_Instances++;

		//m_RenderingContext = RenderingContext::Create();
		//m_RenderingContext->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data); //So we can access/get to the data in lambda functions
		if (properties.CustomPos) glfwSetWindowPos(m_Window, properties.X, properties.Y);

		//Event system
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;
				
			Ref<Event> event = RefHelper::Create<WindowResizeEvent>(width, height);
			data.CallBack(event);
			Application::Get().HandleEvents();
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				
			Ref<Event> event = RefHelper::Create<WindowCloseEvent>();
			data.CallBack(event);
			Application::Get().HandleEvents();
		});


		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				Ref<Event> event = RefHelper::Create<KeyPressedEvent>(key, 0);
				data.CallBack(event);
				break;
			}
			case GLFW_RELEASE:
			{
				Ref<Event> event = RefHelper::Create<KeyReleasedEvent>(key);
				data.CallBack(event);
				break;
			}
			case GLFW_REPEAT:
			{
				Ref<Event> event = RefHelper::Create<KeyPressedEvent>(key, 1);
				data.CallBack(event);
				break;
			}
			Application::Get().HandleEvents();
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			Ref<Event> event = RefHelper::Create<KeyTypedEvent>(keycode);
			data.CallBack(event);
			Application::Get().HandleEvents();
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				Ref<Event> event = RefHelper::Create<MouseButtonPressedEvent>(button);
				data.CallBack(event);
				break;
			}
			case GLFW_RELEASE:
			{
				Ref<Event> event = RefHelper::Create<MouseButtonReleasedEvent>(button);
				data.CallBack(event);
				break;
			}
			Application::Get().HandleEvents();
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			Ref<Event> event = RefHelper::Create<MouseScrolledEvent>((float)xOffset, (float)yOffset);
			data.CallBack(event);
			Application::Get().HandleEvents();
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			Ref<Event> event = RefHelper::Create<MouseMovedEvent>((float)xPos, (float)yPos);
			data.CallBack(event);
			Application::Get().HandleEvents();
		});


		return false;
	}

	void WindowsWindow::Shutdown()
	{
		//m_RenderingContext->Destroy();

		glfwDestroyWindow(m_Window);
		s_Instances--;

		if (s_Instances == 0)
			glfwTerminate();
	}

	void WindowsWindow::ErrorCallBack(int errorCode, const char* description)
	{
		LV_LOG_ERROR("[GLFW]: ({0}), {1}", errorCode, description);
	}

	void SetupAPIWindowHints()
	{
		/*
		switch (Renderer::GetAPI())
		{
		case RenderingAPI::Vulkan:
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			break;

		default:
			LV_LOG_ERROR("Invalid API selected.");
			break;
		}
		*/
	}

}