#pragma once

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/Buffers.hpp"
#include "Swift/Renderer/Descriptors.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Swift
{

	// TODO: Add specifications (CPU/GPU, etc...)

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(void* data, size_t size);
		virtual ~VulkanVertexBuffer();

		void Bind(Ref<CommandBuffer> commandBuffer) override;

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_BufferAllocation = VK_NULL_HANDLE;

		size_t m_BufferSize = 0;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~VulkanIndexBuffer();

		void Bind(Ref<CommandBuffer> commandBuffer) const;

		inline uint32_t GetCount() const override { return m_Count; }

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_BufferAllocation = VK_NULL_HANDLE;

		uint32_t m_Count = 0;
	};

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(size_t dataSize);
		virtual ~VulkanUniformBuffer();

		void SetData(void* data, size_t size, size_t offset) override;

		void Upload(Ref<DescriptorSet> set, Descriptor element) override;

	private:
		std::vector<VkBuffer> m_Buffers = { };
		std::vector<VmaAllocation> m_Allocations = { };

		size_t m_Size = 0;
	};

	class VulkanDynamicUniformBuffer : public DynamicUniformBuffer
	{
	public:
		VulkanDynamicUniformBuffer(uint32_t elements, size_t sizeOfOneElement);
		virtual ~VulkanDynamicUniformBuffer();

		void SetData(void* data, size_t size) override;

		void SetDataIndexed(uint32_t index, void* data, size_t size) override;
		void UploadIndexedData() override;

		inline uint32_t GetAmountOfElements() const override { return m_ElementCount; }
		inline size_t GetAlignment() const override { return m_AlignmentOfOneElement; }

		void Upload(Ref<DescriptorSet> set, Descriptor element) override;
		void Upload(Ref<DescriptorSet> set, Descriptor element, size_t offset) override;

	private:
		std::vector<VkBuffer> m_Buffers = { };
		std::vector<VmaAllocation> m_Allocations = { };

		uint32_t m_ElementCount = 0;
		size_t m_SizeOfOneElement = 0;
		size_t m_AlignmentOfOneElement = 0;

		std::vector<std::pair<void*, size_t>> m_IndexedData = { };
	};

	class VulkanStorageBuffer : public StorageBuffer
	{
	public:
		VulkanStorageBuffer(size_t dataSize);
		virtual ~VulkanStorageBuffer();

		void SetData(void* data, size_t size, size_t offset) override;

		void* StartRetrieval() override;
		void EndRetrieval() override;

		size_t GetSize() const override { return m_Size; }

		void Upload(Ref<DescriptorSet> set, Descriptor element) override;

	private:
		std::vector<VkBuffer> m_Buffers = { };
		std::vector<VmaAllocation> m_Allocations = { };

		size_t m_Size = 0;
	};

}