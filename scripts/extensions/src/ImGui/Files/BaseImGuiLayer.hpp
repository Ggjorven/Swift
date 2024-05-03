#pragma once

#include "Lavender/Core/Layer.hpp"

namespace Lavender
{

	class Application;

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