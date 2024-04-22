#pragma once

#include "Swift/Core/Window.hpp"

#include <GLFW/glfw3.h>

namespace Swift
{

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowSpecification& properties);
		virtual ~WindowsWindow();

		void SetEventCallBack(EventCallBackFunction func) override { m_Data.CallBack = func; }

		void OnUpdate() override;
		void OnRender() override;

		uint32_t GetWidth() const override { return m_Data.Width; }
		uint32_t GetHeight() const override { return m_Data.Height; }

		uint32_t GetPositionX() const override;
		uint32_t GetPositionY() const override;

		uint32_t GetMonitorWidth() const override;
		uint32_t GetMonitorHeight() const override;

		void SetVSync(bool enabled) override;
		bool IsVSync() const override { return m_Data.VSync; }

		void SetTitle(const std::string& title) override;

		void* GetNativeWindow() const override { return (void*)m_Window; }

	private:
		static void ErrorCallBack(int errorCode, const char* description);

	private:
		static bool s_GLFWinitialized;
		static uint32_t s_Instances;

		GLFWwindow* m_Window = nullptr;
		WindowData m_Data = {};

	};

}