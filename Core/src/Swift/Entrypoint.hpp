#pragma once

// To be defined by user.
extern Swift::Application* Swift::CreateApplication(int argc, char* argv[]);

#if !defined(APP_DIST) // Non Dist build on all Platforms

int main(int argc, char* argv[])
{
	Swift::Application* app = Swift::CreateApplication(argc, argv);
	app->Run();
	delete app;
	return 0;
}

#elif defined(APP_PLATFORM_WINDOWS) // Dist on Windows
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	Swift::Application* app = Swift::CreateApplication(__argc, __argv);
    app->Run();
    delete app;
    return 0;
} 
#else // Dist on all other platforms // Maybe fix this? Or maybe this works? // TODO(Jorben): Test this on MacOS and Linux

int main(int argc, char* argv[])
{
	Swift::Application* app = Swift::CreateApplication(argc, argv);
	app->Run();
	delete app;
	return 0;
}

#endif
