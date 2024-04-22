#pragma once

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/Descriptors.hpp"

#include <Vulkan/vulkan.h>

namespace Swift
{

	class VulkanPipeline;

	VkDescriptorType DescriptorTypeToVulkanDescriptorType(DescriptorType type);

	class VulkanDescriptorSet : public DescriptorSet
	{
	public:
		VulkanDescriptorSet(Descriptor::SetID setID, const std::vector<VkDescriptorSet>& sets);
		virtual ~VulkanDescriptorSet() = default;

		void Bind(Ref<Pipeline> pipeline, Ref<CommandBuffer> commandBuffer, PipelineBindPoint bindPoint, const std::vector<uint32_t>& dynamicOffsets) override;

		inline Descriptor::SetID GetSetID() const { return m_SetID; }
		inline VkDescriptorSet GetVulkanSet(uint32_t index) { return m_Sets[index]; }
	
		static Ref<DescriptorSet> Create(Descriptor::SetID setID, const std::vector<VkDescriptorSet>& sets);

	private:
		Descriptor::SetID m_SetID = 0;

		// Note(Jorben): One for every frame in flight
		std::vector<VkDescriptorSet> m_Sets = { };
	};

	class VulkanDescriptorSets : public DescriptorSets
	{
	public:
		VulkanDescriptorSets(const std::initializer_list<AmountGroup>& sets);
		virtual ~VulkanDescriptorSets();

		void SetAmount(Descriptor::SetID setID, uint32_t amount) override;
		uint32_t GetAmount(Descriptor::SetID setID) const override;
		
		DescriptorSetLayout& GetLayout(Descriptor::SetID setID) override;
		std::vector<Ref<DescriptorSet>>& GetSets(Descriptor::SetID setID) override;

	private:
		void CreateDescriptorSetLayout(Descriptor::SetID setID);
		void CreateDescriptorPool(Descriptor::SetID setID, uint32_t amount);
		void CreateDescriptorSets(Descriptor::SetID setID, uint32_t amount);
		void ConvertToVulkanDescriptorSets(Descriptor::SetID setID, uint32_t amount, std::vector<VkDescriptorSet>& sets);

	private:
		Dict<Descriptor::SetID, DescriptorSetLayout> m_OriginalLayouts = { };
		Dict<Descriptor::SetID, std::vector<Ref<DescriptorSet>>> m_DescriptorSets = { };

		Dict<Descriptor::SetID, VkDescriptorSetLayout> m_DescriptorLayouts = { };
		Dict<Descriptor::SetID, VkDescriptorPool> m_DescriptorPools = { };

		friend class VulkanPipeline;
	};

}