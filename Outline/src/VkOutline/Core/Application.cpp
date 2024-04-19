#include "vkpch.h"
#include "Application.hpp"

#include <GLFW/glfw3.h>

#include "VkOutline/Core/Logging.hpp"
#include "VkOutline/Core/Input/Input.hpp"

//#include "VkOutline/Renderer/Renderer.hpp"

#include "VkOutline/Utils/Profiler.hpp"

namespace VkOutline
{

	Application* Application::s_Instance = nullptr;
	static bool s_Initialized = false;

	Application::Application(const ApplicationSpecification& appInfo)
		: m_AppInfo(appInfo)
	{
		Init(appInfo);
	}

	Application::~Application()
	{
		Input::Destroy();

		//Renderer::Wait();

		for (Layer* layer : m_LayerStack)
		{
			layer->OnDetach();
			delete layer;
		}

		//Renderer::Destroy();
		m_Window->Shutdown();
	}

	void Application::SetInitialized(bool value)
	{
		s_Initialized = value;
	}

	bool Application::Initialized()
	{
		return s_Initialized;
	}

	void Application::OnEvent(Ref<Event>& e)
	{
		m_EventQueue.push(e);
	}

	void Application::Run()
	{
		while (m_Running)
		{
			// Delta Time
			float currentTime = (float)Utils::ToolKit::GetTime();
			static float lastTime = 0.0f;

			float deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			// Update & Render
			m_Window->OnUpdate();
			HandleEvents(); // TODO: Remove??
			{
				APP_PROFILE_SCOPE("Renderer::Begin");
				//Renderer::BeginFrame();
			}
			{
				APP_PROFILE_SCOPE("Update & Render");
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate(deltaTime);
					if (!m_Minimized) layer->OnRender();
				}
			}

			{
				APP_PROFILE_SCOPE("Renderer::End");
				//Renderer::EndFrame();
				m_Window->OnRender();
			}
		}
	}

	void Application::AddLayer(Layer* layer)
	{
		m_LayerStack.AddLayer(layer);
	}

	void Application::AddOverlay(Layer* layer)
	{
		m_LayerStack.AddOverlay(layer);
	}

	void Application::Init(const ApplicationSpecification& appInfo)
	{
		s_Instance = this;

		Input::Init();
		Log::Init();
		//Renderer::SetSpecification(appInfo.RenderSpecs);

		m_Window = Window::Create();
		m_Window->Init(appInfo.WindowSpecs);
		m_Window->SetEventCallBack(LV_BIND_EVENT_FN(Application::OnEvent));

		//Renderer::Init();
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return true;
		}

		//Renderer::OnResize(e.GetWidth(), e.GetHeight());

		m_Minimized = false;
		return false;
	}

	void Application::HandleEvents()
	{
		while (!m_EventQueue.empty())
		{
			Ref<Event> rawEvent = m_EventQueue.front();
			Event& e = *rawEvent;

			EventHandler handler(e);

			handler.Handle<WindowCloseEvent>(LV_BIND_EVENT_FN(Application::OnWindowClose));
			handler.Handle<WindowResizeEvent>(LV_BIND_EVENT_FN(Application::OnWindowResize));

			for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
			{
				if (e.Handled)
					break;
				(*it)->OnEvent(e);
			}

			m_EventQueue.pop();
		}
	}

}