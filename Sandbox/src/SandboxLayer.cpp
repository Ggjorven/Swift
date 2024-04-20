#include "SandboxLayer.hpp"

#include <Swift/Core/Application.hpp>

void SandboxLayer::OnAttach()
{
}

void SandboxLayer::OnDetach()
{
}

void SandboxLayer::OnUpdate(float deltaTime)
{
	static float timer = 0.0f;
	static uint32_t FPS = 0u;
	timer += deltaTime;
	FPS += 1u;

	if (timer >= 1.0f)
	{
		Application::Get().GetWindow().SetTitle(fmt::format("SandboxApp | FPS: {0}", FPS));
		timer = 0.0f;
		FPS = 0u;
	}
}

void SandboxLayer::OnRender()
{
}

void SandboxLayer::OnEvent(Event& event)
{
}
