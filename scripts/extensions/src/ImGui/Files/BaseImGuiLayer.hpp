#pragma once

#include "Swift/Core/Layer.hpp"

namespace Swift
{

	class Application;

	// Note(Jorben): The ImGui renderpass only loads from the previous colour attachment and doesn't clear the screen. // TODO: Fix?
	class BaseImGuiLayer : public Layer
	{
	public:
		BaseImGuiLayer() = default;
		virtual ~BaseImGuiLayer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		static BaseImGuiLayer* Create();

		friend class Application;
	};

}