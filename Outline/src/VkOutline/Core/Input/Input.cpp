#include "vkpch.h"
#include "Input.hpp"

#include "VkOutline/Core/Logging.hpp"

#include "VkOutline/Platforms/Windows/WindowsInput.hpp"

namespace VkOutline
{

	Input* Input::s_Instance = nullptr;

	void Input::Init()
	{
		#ifdef APP_PLATFORM_WINDOWS
		s_Instance = new WindowsInput();
		#endif
	}

	void Input::Destroy()
	{
		delete s_Instance;
		s_Instance = nullptr;
	}

}