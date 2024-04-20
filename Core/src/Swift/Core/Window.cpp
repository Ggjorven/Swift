#include "swpch.h"
#include "Window.hpp"

#include "Swift/Platforms/Windows/WindowsWindow.hpp"

namespace Swift
{

	std::unique_ptr<Window> Window::Create()
	{
		#ifdef APP_PLATFORM_WINDOWS
		return std::make_unique<WindowsWindow>();
		#endif

		// TODO: Add all the platforms
	}

}