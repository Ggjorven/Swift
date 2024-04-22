#include "swpch.h"
#include "RenderPass.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanRenderPass.hpp"

namespace Swift
{

	Ref<RenderPass> RenderPass::Create(RenderPassSpecification specs, Ref<CommandBuffer> commandBuffer)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanRenderPass>(specs, commandBuffer);

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

}