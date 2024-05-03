#pragma once

#include <Swift/Core/Layer.hpp>

using namespace Swift;

class SandboxLayer : public Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(float deltaTime);
	void OnRender();
	void OnEvent(Event& e);

private:
};