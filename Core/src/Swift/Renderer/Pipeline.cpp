#include "swpch.h"
#include "Pipeline.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Shader.hpp"
#include "Swift/Renderer/Renderer.hpp"
#include "Swift/Renderer/RenderPass.hpp"
#include "Swift/Renderer/Descriptors.hpp"

#include "Swift/Vulkan/VulkanPipeline.hpp"

namespace Swift
{

	Ref<Pipeline> Pipeline::Create(PipelineSpecification specs, Ref<DescriptorSets> sets, Ref<Shader> shader, Ref<RenderPass> renderpass)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanPipeline>(specs, sets, shader, renderpass);

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

	Ref<Pipeline> Pipeline::Create(PipelineSpecification specs, Ref<DescriptorSets> sets, Ref<ComputeShader> shader)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanPipeline>(specs, sets, shader);

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

}