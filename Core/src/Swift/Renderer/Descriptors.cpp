#include "swpch.h"
#include "Descriptors.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanDescriptors.hpp"

namespace Swift
{

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Specifications 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Descriptor::Descriptor(DescriptorType type, uint32_t binding, const std::string& name, ShaderStage stage, uint32_t count)
		: Type(type), Binding(binding), Name(name), Stage(stage), Count(count)
	{
	}

	DescriptorSetLayout::DescriptorSetLayout(Descriptor::SetID set, const std::vector<Descriptor>& descriptors)
		: Set(set)
	{
		for (auto& descriptor : descriptors)
			Descriptors[descriptor.Name] = descriptor;
	}

	DescriptorSetLayout::DescriptorSetLayout(Descriptor::SetID set, const std::initializer_list<Descriptor>& descriptors)
		: Set(set)
	{
		for (auto& descriptor : descriptors)
			Descriptors[descriptor.Name] = descriptor;
	}

	Descriptor DescriptorSetLayout::GetDescriptorByName(const std::string& name)
	{
		auto it = Descriptors.find(name);
		if (it == Descriptors.end())
		{
			APP_LOG_ERROR("Failed to find descriptor by name: '{0}'", name);
			return {};
		}

		return Descriptors[name];
	}

	std::unordered_set<DescriptorType> DescriptorSetLayout::UniqueTypes() const
	{
		std::unordered_set<DescriptorType> unique = { };

		for (const auto& e : Descriptors)
			unique.insert(e.second.Type);

		return unique;
	}

	uint32_t DescriptorSetLayout::AmountOf(DescriptorType type) const
	{
		uint32_t count = 0;

		for (const auto& e : Descriptors)
		{
			if (e.second.Type == type)
				count++;
		}

		return count;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Classes 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	DescriptorSets::AmountGroup::AmountGroup(uint32_t amount, DescriptorSetLayout layout)
		: Amount(amount), Layout(layout)
	{
	}
	
	Ref<DescriptorSets> DescriptorSets::Create(const std::initializer_list<AmountGroup>& sets)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanDescriptorSets>(sets);

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

}
