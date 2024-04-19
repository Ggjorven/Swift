#include "vkpch.h"
#include "Window.hpp"

#include "VkOutline/Platforms/Windows/WindowsWindow.hpp"

namespace VkOutline
{

	std::unique_ptr<Window> Window::Create()
	{
		#ifdef APP_PLATFORM_WINDOWS
		return std::make_unique<WindowsWindow>();
		#endif

		// TODO(Jorben): Add all the platforms
	}

}