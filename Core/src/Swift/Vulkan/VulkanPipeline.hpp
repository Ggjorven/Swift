#pragma once

#include <memory>
#include <unordered_map>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/Shader.hpp"
#include "Swift/Renderer/Pipeline.hpp"
#include "Swift/Renderer/RenderPass.hpp"
#include "Swift/Renderer/Descriptors.hpp"

#include <vulkan/vulkan.h>

namespace Swift
{

	class VulkanDescriptorSets;

	VkPipelineBindPoint PipelineBindPointToVulkanBindPoint(PipelineBindPoint bindPoint);

	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(PipelineSpecification specs, Ref<DescriptorSets> sets, Ref<Shader> shader, Ref<RenderPass> renderpass);
		VulkanPipeline(PipelineSpecification specs, Ref<DescriptorSets> sets, Ref<ComputeShader> shader);
		virtual ~VulkanPipeline();

		void Use(Ref<CommandBuffer> commandBuffer, PipelineBindPoint bindPoint) override;

		inline PipelineSpecification& GetSpecification() override { return m_Specification; };
		inline Ref<DescriptorSets> GetDescriptorSets() override { return m_Sets; }

		inline VkPipelineLayout GetVulkanLayout() { return m_PipelineLayout; }

	private:
		void CreateGraphicsPipeline();
		void CreateComputePipeline();
		void CreateRayTracingPipeline(); // TODO: Implement

		VkVertexInputBindingDescription GetBindingDescription();
		std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

	private:
		Ref<Shader> m_Shader = nullptr;
		Ref<ComputeShader> m_ComputeShader = nullptr;
		Ref<RenderPass> m_RenderPass = nullptr;

		PipelineSpecification m_Specification = {};
		Ref<DescriptorSets> m_Sets = nullptr;

		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		friend class VulkanDescriptorSets;
	};

}