#pragma once

#include <string>
#include <functional>
#include <memory>

#include "Swift/Core/Core.hpp"
#include "Swift/Core/Events.hpp"

#include "Swift/Renderer/Renderer.hpp"

namespace Swift
{

	typedef std::function<void(Event&)> EventCallBackFunction;

	struct WindowSpecification
	{
		std::string Name;
		uint32_t Width;
		uint32_t Height;

		//bool Titlebar = true; // TODO(Jorben): Implement using an updated GLFW branch
		bool VSync = true;

		bool CustomPos = false;
		uint32_t X = 0u;
		uint32_t Y = 0u;

		WindowSpecification(std::string name = "Swift Window", uint32_t width = 1280u, uint32_t height = 720u)
			: Name(name), Width(width), Height(height)
		{
		}
	};

	struct WindowData
	{
		std::string Name;
		uint32_t Width;
		uint32_t Height;

		bool VSync = false;
		EventCallBackFunction CallBack;

		WindowData(std::string name = "Swift Window", uint32_t width = 1280u, uint32_t height = 720u)
			: Name(name), Width(width), Height(height)
		{
		}
	};

	class Window
	{
	public:
		Window() = default;
		virtual ~Window() = default;

		virtual void SetEventCallBack(EventCallBackFunction func) = 0;

		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;

		// Actual window size
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual uint32_t GetPositionX() const = 0;
		virtual uint32_t GetPositionY() const = 0;

		virtual uint32_t GetMonitorWidth() const = 0;
		virtual uint32_t GetMonitorHeight() const = 0;

		// Extra
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void SetTitle(const std::string& title) = 0;

		virtual void* GetNativeWindow() const = 0;

		static std::unique_ptr<Window> Create(const WindowSpecification& properties = WindowSpecification());
	};

}