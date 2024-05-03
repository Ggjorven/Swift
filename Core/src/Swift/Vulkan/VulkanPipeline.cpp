#include "swpch.h"
#include "VulkanPipeline.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanShader.hpp"
#include "Swift/Vulkan/VulkanRenderPass.hpp"
#include "Swift/Vulkan/VulkanCommandBuffer.hpp"
#include "Swift/Vulkan/VulkanDescriptors.hpp"

namespace Swift
{

	static VkFormat DataTypeToVulkanType(DataType type);

	VulkanPipeline::VulkanPipeline(PipelineSpecification specs, Ref<DescriptorSets> sets, Ref<Shader> shader, Ref<RenderPass> renderpass)
		: m_Specification(specs), m_Sets(sets), m_Shader(shader), m_RenderPass(renderpass)
	{
		CreateGraphicsPipeline();
	}

	VulkanPipeline::VulkanPipeline(PipelineSpecification specs, Ref<DescriptorSets> sets, Ref<ComputeShader> shader)
		: m_Specification(specs), m_Sets(sets), m_ComputeShader(shader)
	{
		CreateComputePipeline();
	}

	VulkanPipeline::~VulkanPipeline()
	{
		auto pipeline = m_GraphicsPipeline;
		auto layout = m_PipelineLayout;

		Renderer::SubmitFree([pipeline, layout]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, layout, nullptr);
		});
	}

	void VulkanPipeline::Use(Ref<CommandBuffer> commandBuffer, PipelineBindPoint bindPoint)
	{
		auto cmdBuf = RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer);

		vkCmdBindPipeline(cmdBuf->GetVulkanCommandBuffer(Renderer::GetCurrentFrame()), PipelineBindPointToVulkanBindPoint(bindPoint), m_GraphicsPipeline);
	}

	void VulkanPipeline::CreateGraphicsPipeline()
	{
		auto vkShader = RefHelper::RefAs<VulkanShader>(m_Shader);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { };
		auto vertex = vkShader->GetVertexShader();
		if (vertex)
		{
			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertex;
			vertShaderStageInfo.pName = "main";

			shaderStages.push_back(vertShaderStageInfo);
		}

		auto fragment = vkShader->GetFragmentShader();
		if (fragment)
		{
			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragment;
			fragShaderStageInfo.pName = "main";

			shaderStages.push_back(fragShaderStageInfo);
		}

		auto bindingDescription = GetBindingDescription();
		auto attributeDescriptions = GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		if (!m_Specification.Bufferlayout.GetElements().empty())
		{
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		}
		else
		{
			vertexInputInfo.vertexBindingDescriptionCount = 0;
			vertexInputInfo.vertexAttributeDescriptionCount = 0;
			vertexInputInfo.pVertexAttributeDescriptions = nullptr;
			vertexInputInfo.pVertexBindingDescriptions = nullptr;
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = (VkPolygonMode)(m_Specification.Polygonmode);
		rasterizer.lineWidth = m_Specification.LineWidth;
		rasterizer.cullMode = (VkCullModeFlags)(m_Specification.Cullingmode);
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = m_Specification.Blending; // Note(Jorben): Set true for transparancy

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		// Descriptor layouts
		auto vkDescriptorSets = RefHelper::RefAs<VulkanDescriptorSets>(m_Sets);

		std::vector<VkDescriptorSetLayout> descriptorLayouts = { };
		descriptorLayouts.reserve(vkDescriptorSets->m_DescriptorLayouts.size());

		for (auto& pair : vkDescriptorSets->m_DescriptorLayouts)
			descriptorLayouts.push_back(pair.second);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();

		auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create pipeline layout!");

		// Create the actual graphics pipeline (where we actually use the shaders and other info)
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = RefHelper::RefAs<VulkanRenderPass>(m_RenderPass)->GetVulkanRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create graphics pipeline!");
	}

	void VulkanPipeline::CreateComputePipeline()
	{
		auto vkComputeShader = RefHelper::RefAs<VulkanComputeShader>(m_ComputeShader);

		VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = vkComputeShader->GetComputeShader();
		computeShaderStageInfo.pName = "main";

		auto vkDescriptorSets = RefHelper::RefAs<VulkanDescriptorSets>(m_Sets);

		std::vector<VkDescriptorSetLayout> descriptorLayouts;
		descriptorLayouts.reserve(vkDescriptorSets->m_DescriptorLayouts.size());

		for (auto& pair : vkDescriptorSets->m_DescriptorLayouts)
			descriptorLayouts.push_back(pair.second);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();

		auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create pipeline layout!");

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = computeShaderStageInfo;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create compute pipeline!");
	}

	void VulkanPipeline::CreateRayTracingPipeline() // TODO: Implement
	{
		
	}



	VkVertexInputBindingDescription VulkanPipeline::GetBindingDescription()
	{
		VkVertexInputBindingDescription description = {};
		description.binding = 0;
		description.stride = m_Specification.Bufferlayout.GetStride();
		description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return description;
	}

	std::vector<VkVertexInputAttributeDescription> VulkanPipeline::GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
		attributeDescriptions.resize(m_Specification.Bufferlayout.GetElements().size());

		auto& elements = m_Specification.Bufferlayout.GetElements();
		for (size_t i = 0; i < elements.size(); i++)
		{
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = elements[i].Location;
			attributeDescriptions[i].format = DataTypeToVulkanType(elements[i].Type);
			attributeDescriptions[i].offset = (uint32_t)elements[i].Offset;
		}

		return attributeDescriptions;
	}

	static VkFormat DataTypeToVulkanType(DataType type)
	{
		switch (type)
		{
		case DataType::Float:   return VK_FORMAT_R32_SFLOAT;
		case DataType::Float2:  return VK_FORMAT_R32G32_SFLOAT;
		case DataType::Float3:  return VK_FORMAT_R32G32B32_SFLOAT;
		case DataType::Float4:  return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DataType::Mat3:    return VK_FORMAT_R32G32B32_SFLOAT;		// Assuming Mat3 is represented as 3 vec3s
		case DataType::Mat4:    return VK_FORMAT_R32G32B32A32_SFLOAT;	// Assuming Mat4 is represented as 4 vec4s
		case DataType::Int:     return VK_FORMAT_R32_SINT;			
		case DataType::Int2:    return VK_FORMAT_R32G32_SINT;
		case DataType::Int3:    return VK_FORMAT_R32G32B32_SINT;
		case DataType::Int4:    return VK_FORMAT_R32G32B32A32_SINT;
		case DataType::Bool:    return VK_FORMAT_R8_UINT;
		}

		return VK_FORMAT_UNDEFINED;
	}

	VkPipelineBindPoint PipelineBindPointToVulkanBindPoint(PipelineBindPoint bindPoint)
	{
		switch (bindPoint)
		{
		case PipelineBindPoint::Graphics:		return VK_PIPELINE_BIND_POINT_GRAPHICS;
		case PipelineBindPoint::Compute:		return VK_PIPELINE_BIND_POINT_COMPUTE;
		case PipelineBindPoint::RayTracingKHR:	return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
		case PipelineBindPoint::RayTracingNV:	return VK_PIPELINE_BIND_POINT_RAY_TRACING_NV;
		}

		return VK_PIPELINE_BIND_POINT_MAX_ENUM;
	}

}