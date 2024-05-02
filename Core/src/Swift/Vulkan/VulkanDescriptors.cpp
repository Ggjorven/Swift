#include "swpch.h"
#include "VulkanDescriptors.hpp"

#include "Swift/Core/Logging.hpp"
#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanPipeline.hpp"
#include "Swift/Vulkan/VulkanCommandBuffer.hpp"

namespace Swift
{

	static VkShaderStageFlags UniformStageFlagsToVulkanStageFlags(ShaderStage flags);

	VulkanDescriptorSet::VulkanDescriptorSet(Descriptor::SetID setID, const std::vector<VkDescriptorSet>& sets)
		: m_SetID(setID), m_Sets(sets)
	{
	}

	void VulkanDescriptorSet::Bind(Ref<Pipeline> pipeline, Ref<CommandBuffer> commandBuffer, PipelineBindPoint bindPoint, const std::vector<uint32_t>& dynamicOffsets)
	{
		APP_PROFILE_SCOPE("VulkanDescriptorSet::Bind");

		auto vkPipelineLayout = RefHelper::RefAs<VulkanPipeline>(pipeline)->GetVulkanLayout();
		auto vkCmdBuf = RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer)->GetVulkanCommandBuffer(Renderer::GetCurrentFrame());

		vkCmdBindDescriptorSets(vkCmdBuf, PipelineBindPointToVulkanBindPoint(bindPoint), vkPipelineLayout, m_SetID, 1, &m_Sets[Renderer::GetCurrentFrame()], (uint32_t)dynamicOffsets.size(), dynamicOffsets.data());
	}

	Ref<DescriptorSet> VulkanDescriptorSet::Create(Descriptor::SetID setID, const std::vector<VkDescriptorSet>& sets)
	{
		return RefHelper::Create<VulkanDescriptorSet>(setID, sets);
	}

	VulkanDescriptorSets::VulkanDescriptorSets(const std::initializer_list<AmountGroup>& sets)
	{
		for (auto& layout : sets)
		{
			m_OriginalLayouts[layout.Layout.Set] = layout.Layout;

			CreateDescriptorSetLayout(layout.Layout.Set);
			CreateDescriptorPool(layout.Layout.Set, layout.Amount);
			CreateDescriptorSets(layout.Layout.Set, layout.Amount);
		}
	}

	VulkanDescriptorSets::~VulkanDescriptorSets()
	{
		auto pools = m_DescriptorPools;
		auto layouts = m_DescriptorLayouts;

		Renderer::SubmitFree([pools, layouts]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

			for (auto& pool : pools)
				vkDestroyDescriptorPool(device, pool.second, nullptr);

			for (auto& layout : layouts)
				vkDestroyDescriptorSetLayout(device, layout.second, nullptr);
		});
	}

	void VulkanDescriptorSets::SetAmount(Descriptor::SetID setID, uint32_t amount)
	{
		auto pool = m_DescriptorPools[setID];
		Renderer::SubmitFree([pool]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

			vkDestroyDescriptorPool(device, pool, nullptr);
		});

		CreateDescriptorPool(setID, amount);
		CreateDescriptorSets(setID, amount);
	}

	uint32_t VulkanDescriptorSets::GetAmount(Descriptor::SetID setID) const
	{
		auto it = m_DescriptorSets.find(setID);
		if (it == m_DescriptorSets.end())
		{
			APP_LOG_ERROR("Failed to find descriptor set by ID: {0}", setID);
			return 0;
		}

		return (uint32_t)it->second.size();
	}

	DescriptorSetLayout& VulkanDescriptorSets::GetLayout(Descriptor::SetID setID)
	{
		auto it = m_OriginalLayouts.find(setID);
		if (it == m_OriginalLayouts.end())
		{
			APP_LOG_ERROR("Failed to find descriptor set by ID: {0}", setID);
		}

		return it->second;
	}

	std::vector<Ref<DescriptorSet>>& VulkanDescriptorSets::GetSets(Descriptor::SetID setID)
	{
		auto it = m_DescriptorSets.find(setID);
		if (it == m_DescriptorSets.end())
		{
			APP_LOG_ERROR("Failed to find descriptor set by ID: {0}", setID);
		}

		return it->second;
	}

	void VulkanDescriptorSets::CreateDescriptorSetLayout(Descriptor::SetID setID)
	{
		std::vector<VkDescriptorSetLayoutBinding> layouts = { };

		for (auto& element : m_OriginalLayouts[setID].Descriptors)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = element.second.Binding;
			layoutBinding.descriptorType = DescriptorTypeToVulkanDescriptorType(element.second.Type);
			layoutBinding.descriptorCount = element.second.Count;
			layoutBinding.stageFlags = UniformStageFlagsToVulkanStageFlags(element.second.Stage);
			layoutBinding.pImmutableSamplers = nullptr; // Optional

			layouts.push_back(layoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = (uint32_t)layouts.size();
		layoutInfo.pBindings = layouts.data();

		m_DescriptorLayouts[setID] = VK_NULL_HANDLE;
		if (vkCreateDescriptorSetLayout(((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice(), &layoutInfo, nullptr, &m_DescriptorLayouts[setID]) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create descriptor set layout!");
	}

	void VulkanDescriptorSets::CreateDescriptorPool(Descriptor::SetID setID, uint32_t amount)
	{
		auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

		// Note(Jorben): Just for myself, the poolSizes is just the amount of elements of a certain type to able to allocate per pool
		std::vector<VkDescriptorPoolSize> poolSizes = { };
		poolSizes.resize(m_OriginalLayouts[setID].UniqueTypes().size());
		poolSizes.clear(); // Note(Jorben): For some reason without this line there is a VK_SAMPLER or something in the list.

		for (auto& type : m_OriginalLayouts[setID].UniqueTypes())
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = DescriptorTypeToVulkanDescriptorType(type);
			poolSize.descriptorCount = m_OriginalLayouts[setID].AmountOf(type) * (uint32_t)RendererSpecification::BufferCount * amount;

			poolSizes.push_back(poolSize);
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = (uint32_t)RendererSpecification::BufferCount * amount; // A set for every frame in flight

		m_DescriptorPools[setID] = VK_NULL_HANDLE;
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPools[setID]) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create descriptor pool!");
	}

	void VulkanDescriptorSets::CreateDescriptorSets(Descriptor::SetID setID, uint32_t amount)
	{
		auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

		std::vector<VkDescriptorSet> descriptorSets = { };
		std::vector<VkDescriptorSetLayout> layouts((size_t)RendererSpecification::BufferCount * amount, m_DescriptorLayouts[setID]);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPools[setID];
		allocInfo.descriptorSetCount = (uint32_t)RendererSpecification::BufferCount * amount;
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize((size_t)RendererSpecification::BufferCount * amount);

		VkResult res = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
		if (res != VK_SUCCESS)
			APP_LOG_ERROR("Failed to allocate descriptor sets! Error: {0}", VkResultToString(res));

		ConvertToVulkanDescriptorSets(setID, amount, descriptorSets);
	}

	void VulkanDescriptorSets::ConvertToVulkanDescriptorSets(Descriptor::SetID setID, uint32_t amount, std::vector<VkDescriptorSet>& sets)
	{
		m_DescriptorSets[setID].resize((size_t)amount);

		uint32_t index = 0;
		for (uint32_t i = 0; i < amount; i++)
		{
			std::vector<VkDescriptorSet> setCombo = { };

			for (uint32_t j = 0; j < (uint32_t)RendererSpecification::BufferCount; j++)
				setCombo.push_back(sets[index + j]);

			m_DescriptorSets[setID][i] = RefHelper::Create<VulkanDescriptorSet>(setID, setCombo);
			index += (uint32_t)RendererSpecification::BufferCount;
		}
	}



	VkDescriptorType DescriptorTypeToVulkanDescriptorType(DescriptorType type)
	{
		switch (type)
		{
		case DescriptorType::UniformBuffer:				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case DescriptorType::DynamicUniformBuffer:		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case DescriptorType::Image:						return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case DescriptorType::StorageImage:				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case DescriptorType::StorageBuffer:				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}

	static VkShaderStageFlags UniformStageFlagsToVulkanStageFlags(ShaderStage flags)
	{
		VkShaderStageFlags result = 0;

		if (flags & ShaderStage::Vertex)
			result |= VK_SHADER_STAGE_VERTEX_BIT;
		if (flags & ShaderStage::Fragment)
			result |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if (flags & ShaderStage::Compute)
			result |= VK_SHADER_STAGE_COMPUTE_BIT;

		return result;
	}

}