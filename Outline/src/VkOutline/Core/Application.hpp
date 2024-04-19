#pragma once

#include "VkOutline/Core/Events.hpp"
#include "VkOutline/Core/Layer.hpp"

#include "VkOutline/Core/Window.hpp"

#include <vector>
#include <memory>
#include <queue>
#include <filesystem>

namespace VkOutline
{

	struct ApplicationSpecification
	{
	public:
		WindowSpecification WindowSpecs = { };
		//RendererSpecification RenderSpecs = { };

	public:
		ApplicationSpecification() = default;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& appInfo);
		virtual ~Application();

		static void SetInitialized(bool value);
		static bool Initialized();

		void OnEvent(Ref<Event>& e);

		void Run();
		inline void Close() { m_Running = false; }

		void AddLayer(Layer* layer);
		void AddOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }

		inline bool IsMinimized() const { return m_Minimized; }

		void HandleEvents();
	
	private:
		void Init(const ApplicationSpecification& appInfo);

		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		ApplicationSpecification m_AppInfo;

		std::unique_ptr<Window> m_Window = nullptr;
		bool m_Running = true;
		bool m_Minimized = false;

		std::queue<Ref<Event>> m_EventQueue = { };

		LayerStack m_LayerStack;

	private:
		static Application* s_Instance;

	};


	// Implemented by USER/CLIENT
	Application* CreateApplication(int argc, char* argv[]);

}