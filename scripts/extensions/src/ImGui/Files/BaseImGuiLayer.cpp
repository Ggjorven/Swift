#include "swpch.h"
#include "BaseImGuiLayer.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanImGuiLayer.hpp"

namespace Swift
{

	BaseImGuiLayer* BaseImGuiLayer::Create()
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return new VulkanImGuiLayer();

		default:
			APP_LOG_ERROR("Invalid API selected.");
			break;
		}

		return nullptr;
	}

}