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
};