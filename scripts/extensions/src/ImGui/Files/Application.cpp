#include "swpch.h"
#include "Application.hpp"

#include <GLFW/glfw3.h>

#include "Swift/Core/Logging.hpp"
#include "Swift/Core/Input/Input.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Utils/Profiler.hpp"

namespace Swift
{

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& appInfo)
		: m_AppInfo(appInfo)
	{
		s_Instance = this;
	
		Input::Init();
		Log::Init();

		m_Window = Window::Create(appInfo.WindowSpecs);
		m_Window->SetEventCallBack(APP_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();

        m_ImGuiLayer = BaseImGuiLayer::Create();
        m_LayerStack.AddOverlay((Layer*)m_ImGuiLayer);
	}

	Application::~Application()
	{
		Input::Destroy();

		Renderer::Wait();

		for (Layer* layer : m_LayerStack)
		{
			layer->OnDetach();
			delete layer;
		}

		Renderer::Destroy();
		m_Window.reset();
	}

	void Application::OnEvent(Event& e)
	{
		EventHandler handler(e);

		handler.Handle<WindowCloseEvent>(APP_BIND_EVENT_FN(Application::OnWindowClose));
		handler.Handle<WindowResizeEvent>(APP_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
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
			{
				APP_PROFILE_SCOPE("Renderer::Begin");
				Renderer::BeginFrame();
			}
			{
				APP_PROFILE_SCOPE("Update & Render");
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate(deltaTime);
					if (!m_Minimized) layer->OnRender();
				}
			}

            // ImGui
            if (!m_Minimized)
            {
                APP_PROFILE_SCOPE("ImGui");
                
                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerStack)
                    layer->OnImGuiRender();
                m_ImGuiLayer->End();
            }

			{
				APP_PROFILE_SCOPE("Renderer::End");
				Renderer::EndFrame();
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

		Renderer::OnResize(e.GetWidth(), e.GetHeight());

        m_ImGuiLayer->Resize(e.GetWidth(), e.GetHeight());

		m_Minimized = false;
		return false;
	}

}