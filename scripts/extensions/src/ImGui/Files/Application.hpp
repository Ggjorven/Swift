#pragma once

#include "Swift/Core/Events.hpp"
#include "Swift/Core/Layer.hpp"

#include "Swift/Core/Window.hpp"

#include "Swift/Utils/BaseImGuiLayer.hpp"

#include <vector>
#include <memory>
#include <queue>
#include <filesystem>

namespace Swift
{

	struct ApplicationSpecification
	{
	public:
		WindowSpecification WindowSpecs = { };

	public:
		ApplicationSpecification() = default;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& appInfo);
		virtual ~Application();

		void OnEvent(Event& e);

		void Run();
		inline void Close() { m_Running = false; }

		void AddLayer(Layer* layer);
		void AddOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }

		inline bool IsMinimized() const { return m_Minimized; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		ApplicationSpecification m_AppInfo = {};

		std::unique_ptr<Window> m_Window = nullptr;
		bool m_Running = true;
		bool m_Minimized = false;

        BaseImGuiLayer* m_ImGuiLayer = nullptr;

		LayerStack m_LayerStack = {};

	private:
		static Application* s_Instance;

	};



	// Implemented by USER/CLIENT
	Application* CreateApplication(int argc, char* argv[]);

}