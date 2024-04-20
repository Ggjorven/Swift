#include "swpch.h"
#include "Input.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Platforms/Windows/WindowsInput.hpp"

namespace Swift
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