#include "lvpch.h"
#include "BaseImGuiLayer.hpp"

#include "Lavender/Core/Logging.hpp"

#include "Lavender/Renderer/Renderer.hpp"

#include "Lavender/APIs/Vulkan/VulkanImGuiLayer.hpp"

namespace Lavender
{

	BaseImGuiLayer* BaseImGuiLayer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RenderingAPI::Vulkan:
			return new VulkanImGuiLayer();

		default:
			LV_LOG_ERROR("Invalid API selected.");
			break;
		}

		return nullptr;
	}

}