#pragma once

#include <vector>
#include <utility>
#include <unordered_set>
#include <initializer_list>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/Pipeline.hpp" // Note(Jorben): This is not ideal, maybe remove one day?

namespace Swift
{

	class Pipeline;
	class CommandBuffer;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Specifications 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class DescriptorType : uint8_t
	{
		None = 0, UniformBuffer, DynamicUniformBuffer, Image, StorageImage, StorageBuffer
	};

	enum class ShaderStage : uint8_t
	{
		None = 0, Vertex = BIT(0), Fragment = BIT(1), Compute = BIT(2)
	};
	DEFINE_BITWISE_OPS(ShaderStage)

	// Note(Jorben): You can think of a descriptor as a uniform or some variable in the shader
	struct Descriptor
	{
	public:
		typedef uint32_t SetID;
	public:
		std::string Name = "Empty";
		uint32_t Binding = 0;
		DescriptorType Type = DescriptorType::None;
		uint32_t Count = 1;
		ShaderStage Stage = ShaderStage::None;

		Descriptor() = default;
		Descriptor(DescriptorType type, uint32_t binding, const std::string& name, ShaderStage stage, uint32_t count = 1);
		virtual ~Descriptor() = default;
	};

	struct DescriptorSetLayout
	{
	public:
		Descriptor::SetID Set = 0;
		Dict<std::string, Descriptor> Descriptors = { };

	public:
		DescriptorSetLayout() = default;
		DescriptorSetLayout(Descriptor::SetID set, const std::vector<Descriptor>& descriptors);
		DescriptorSetLayout(Descriptor::SetID set, const std::initializer_list<Descriptor>& descriptors);
		virtual ~DescriptorSetLayout() = default;

		Descriptor GetDescriptorByName(const std::string& name);
		std::unordered_set<DescriptorType> UniqueTypes() const;
		uint32_t AmountOf(DescriptorType type) const;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Classes 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Note(Jorben): This class is only an interface and is not supposed to be created/instantiated by yourself.
	class DescriptorSet
	{
	public:
		DescriptorSet() = default;
		virtual ~DescriptorSet() = default;
	
		virtual void Bind(Ref<Pipeline> pipeline, Ref<CommandBuffer> commandBuffer, PipelineBindPoint bindPoint = PipelineBindPoint::Graphics, const std::vector<uint32_t>& dynamicOffsets = { }) = 0;
	};

	class DescriptorSets
	{
	public:
		struct AmountGroup
		{
		public:
			uint32_t Amount = 0;
			DescriptorSetLayout Layout = {};

		public:
			AmountGroup() = default;
			AmountGroup(uint32_t amount, DescriptorSetLayout layout);
			virtual ~AmountGroup() = default;
		};
	public:
		DescriptorSets() = default;
		virtual ~DescriptorSets() = default;

		virtual void SetAmount(Descriptor::SetID setID, uint32_t amount) = 0;
		virtual uint32_t GetAmount(Descriptor::SetID setID) const = 0;

		virtual DescriptorSetLayout& GetLayout(Descriptor::SetID setID) = 0;
		virtual std::vector<Ref<DescriptorSet>>& GetSets(Descriptor::SetID setID) = 0;

		static Ref<DescriptorSets> Create(const std::initializer_list<AmountGroup>& sets);
	};

}