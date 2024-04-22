#include "swpch.h"
#include "CommandBuffer.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanCommandBuffer.hpp"

namespace Swift
{

	Ref<CommandBuffer> CommandBuffer::Create(CommandBufferSpecification specs)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanCommandBuffer>(specs);

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

}