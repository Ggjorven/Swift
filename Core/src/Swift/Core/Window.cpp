#include "swpch.h"
#include "Window.hpp"

#include "Swift/Platforms/Windows/WindowsWindow.hpp"

namespace Swift
{

	std::unique_ptr<Window> Window::Create(const WindowSpecification& properties)
	{
		#ifdef APP_PLATFORM_WINDOWS
		return std::make_unique<WindowsWindow>(properties);
		#endif

		// TODO: Add all the platforms
	}

}