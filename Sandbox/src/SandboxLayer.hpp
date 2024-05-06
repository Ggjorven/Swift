#pragma once

#include <Swift/Core/Layer.hpp>

using namespace Swift;

class SandboxLayer : public Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(float deltaTime) override;
	void OnRender() override;
	void OnEvent(Event& e) override;
	void OnImGuiRender(); // Purposefully left 'override' out since this function doesn't do anything if ImGui is not installed.
};