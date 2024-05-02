#include "swpch.h"
#include "VulkanBuffers.hpp"

#include "Swift/Core/Logging.hpp"
#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanCommandBuffer.hpp"
#include "Swift/Vulkan/VulkanDescriptors.hpp"

namespace Swift
{

	VulkanVertexBuffer::VulkanVertexBuffer(void* data, size_t size)
		: m_BufferSize(size)
	{
		VulkanAllocator allocator = {};

		m_BufferAllocation = allocator.AllocateBuffer(m_BufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, m_Buffer);

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
		stagingBufferAllocation = allocator.AllocateBuffer(m_BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

		void* mappedData = nullptr;
		allocator.MapMemory(stagingBufferAllocation, mappedData);
		memcpy(mappedData, data, m_BufferSize);
		allocator.UnMapMemory(stagingBufferAllocation);

		allocator.CopyBuffer(stagingBuffer, m_Buffer, m_BufferSize);
		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		auto buffer = m_Buffer;
		auto allocation = m_BufferAllocation;

		Renderer::SubmitFree([buffer, allocation]()
		{
			VulkanAllocator allocator = {};

			if (buffer != VK_NULL_HANDLE)
				allocator.DestroyBuffer(buffer, allocation);
		});
	}

	void VulkanVertexBuffer::Bind(Ref<CommandBuffer> commandBuffer)
	{
		auto cmdBuf = RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer);
		
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuf->GetVulkanCommandBuffer(Renderer::GetCurrentFrame()), 0, 1, &m_Buffer, offsets);
	}



	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		VulkanAllocator allocator = {};

		VkDeviceSize bufferSize = sizeof(uint32_t) * count;
		m_BufferAllocation = allocator.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, m_Buffer);

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
		stagingBufferAllocation = allocator.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

		void* mappedData = nullptr;
		allocator.MapMemory(stagingBufferAllocation, mappedData);
		memcpy(mappedData, indices, bufferSize);
		allocator.UnMapMemory(stagingBufferAllocation);

		allocator.CopyBuffer(stagingBuffer, m_Buffer, bufferSize);
		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		auto buffer = m_Buffer;
		auto allocation = m_BufferAllocation;

		Renderer::SubmitFree([buffer, allocation]()
		{
			VulkanAllocator allocator = {};

			if (buffer != VK_NULL_HANDLE)
				allocator.DestroyBuffer(buffer, allocation);
		});
	}

	void VulkanIndexBuffer::Bind(Ref<CommandBuffer> commandBuffer) const
	{
		auto cmdBuf = RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer);

		vkCmdBindIndexBuffer(cmdBuf->GetVulkanCommandBuffer(Renderer::GetCurrentFrame()), m_Buffer, 0, VK_INDEX_TYPE_UINT32);
	}



	VulkanUniformBuffer::VulkanUniformBuffer(size_t dataSize)
		: m_Size(dataSize)
	{
		uint32_t framesInFlight = (uint32_t)RendererSpecification::BufferCount;
		m_Buffers.resize((size_t)framesInFlight);
		m_Allocations.resize((size_t)framesInFlight);

		VulkanAllocator allocator = {};
		for (size_t i = 0; i < framesInFlight; i++)
			m_Allocations[i] = allocator.AllocateBuffer((VkDeviceSize)dataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_Buffers[i]);
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		auto buffers = m_Buffers;
		auto allocations = m_Allocations;

		Renderer::SubmitFree([buffers, allocations]()
		{
			for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
			{
				VulkanAllocator allocator = {};

				if (buffers[i] != VK_NULL_HANDLE)
					allocator.DestroyBuffer(buffers[i], allocations[i]);
			}
		});
	}

	void VulkanUniformBuffer::SetData(void* data, size_t size, size_t offset)
	{
		APP_PROFILE_SCOPE("VulkanUniformBuffer::SetData");

		if (size + offset > m_Size)
		{
			APP_ASSERT(false, "Data exceeds buffer size in SetData()");
			return;
		}

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			void* mappedMemory = nullptr;
			VulkanAllocator::MapMemory(m_Allocations[i], mappedMemory);
			memcpy(static_cast<uint8_t*>(mappedMemory) + offset, data, size);
			VulkanAllocator::UnMapMemory(m_Allocations[i]);
		}
	}

	void VulkanUniformBuffer::Upload(Ref<DescriptorSet> set, Descriptor element)
	{
		APP_PROFILE_SCOPE("VulkanUniformBuffer::Upload");

		auto vkSet = RefHelper::RefAs<VulkanDescriptorSet>(set);

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = m_Buffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = m_Size;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = vkSet->GetVulkanSet((uint32_t)i);
			descriptorWrite.dstBinding = element.Binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = element.Count;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice(), 1, &descriptorWrite, 0, nullptr);
		}
	}

	

	VulkanDynamicUniformBuffer::VulkanDynamicUniformBuffer(uint32_t elements, size_t sizeOfOneElement)
		: m_ElementCount(elements), m_SizeOfOneElement(sizeOfOneElement)
	{
		size_t uboAlignment = ((VulkanRenderer*)Renderer::GetInstance())->GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment;
		m_AlignmentOfOneElement = (sizeOfOneElement / uboAlignment) * uboAlignment + ((sizeOfOneElement % uboAlignment) > 0 ? uboAlignment : 0);

		uint32_t framesInFlight = (uint32_t)RendererSpecification::BufferCount;
		m_Buffers.resize((size_t)framesInFlight);
		m_Allocations.resize((size_t)framesInFlight);

		VulkanAllocator allocator = {};
		for (size_t i = 0; i < framesInFlight; i++)
			m_Allocations[i] = allocator.AllocateBuffer((VkDeviceSize)(m_ElementCount * m_AlignmentOfOneElement), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_Buffers[i]);

		m_IndexedData.resize((size_t)elements);
	}

	VulkanDynamicUniformBuffer::~VulkanDynamicUniformBuffer()
	{
		auto buffers = m_Buffers;
		auto allocations = m_Allocations;

		Renderer::SubmitFree([buffers, allocations]()
		{
			VulkanAllocator allocator = {};

			for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
			{
				if (buffers[i] != VK_NULL_HANDLE)
					allocator.DestroyBuffer(buffers[i], allocations[i]);
			}
		});
	}

	void VulkanDynamicUniformBuffer::SetData(void* data, size_t size)
	{
		APP_PROFILE_SCOPE("VulkanDynamicUniformBuffer::SetData");

		if (size != m_ElementCount * m_SizeOfOneElement)
		{
			APP_ASSERT(false, "Invalid size passed to SetData()");
			return;
		}

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			void* mappedMemory = nullptr;
			VulkanAllocator::MapMemory(m_Allocations[i], mappedMemory);
			memcpy(mappedMemory, data, size);
			VulkanAllocator::UnMapMemory(m_Allocations[i]);
		}
	}

	void VulkanDynamicUniformBuffer::SetDataIndexed(uint32_t index, void* data, size_t size)
	{
		m_IndexedData[index] = std::make_pair(data, size);
	}

	void VulkanDynamicUniformBuffer::UploadIndexedData()
	{
		APP_PROFILE_SCOPE("VulkanDynamicUniformBuffer::UploadIndexedData");

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			void* mappedMemory = nullptr;
			VulkanAllocator::MapMemory(m_Allocations[i], mappedMemory);

			for (size_t j = 0; j < m_IndexedData.size(); j++)
			{
				void* srcData = m_IndexedData[j].first;
				size_t srcSize = m_IndexedData[j].second;
				size_t copySize = std::min(srcSize, m_AlignmentOfOneElement); // Ensure not to copy more than the aligned size
				memcpy(static_cast<char*>(mappedMemory) + j * m_AlignmentOfOneElement, srcData, copySize);
			}

			VulkanAllocator::UnMapMemory(m_Allocations[i]);
		}

		m_IndexedData.clear();
		m_IndexedData.resize(m_ElementCount);
	}

	void VulkanDynamicUniformBuffer::Upload(Ref<DescriptorSet> set, Descriptor element)
	{
		APP_PROFILE_SCOPE("VulkanUniformBuffer::Upload");

		auto vkSet = RefHelper::RefAs<VulkanDescriptorSet>(set);

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = m_Buffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = m_ElementCount * m_AlignmentOfOneElement;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = vkSet->GetVulkanSet((uint32_t)i);
			descriptorWrite.dstBinding = element.Binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptorWrite.descriptorCount = element.Count;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice(), 1, &descriptorWrite, 0, nullptr);
		}
	}

	void VulkanDynamicUniformBuffer::Upload(Ref<DescriptorSet> set, Descriptor element, size_t offset)
	{
		APP_PROFILE_SCOPE("VulkanUniformBuffer::Upload");

		auto vkSet = RefHelper::RefAs<VulkanDescriptorSet>(set);

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = m_Buffers[i];
			bufferInfo.offset = offset;
			bufferInfo.range = m_AlignmentOfOneElement;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = vkSet->GetVulkanSet((uint32_t)i);
			descriptorWrite.dstBinding = element.Binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptorWrite.descriptorCount = element.Count;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice(), 1, &descriptorWrite, 0, nullptr);
		}
	}



	VulkanStorageBuffer::VulkanStorageBuffer(size_t dataSize)
		: m_Size(dataSize)
	{
		uint32_t framesInFlight = (uint32_t)RendererSpecification::BufferCount;
		m_Buffers.resize((size_t)framesInFlight);
		m_Allocations.resize((size_t)framesInFlight);

		VulkanAllocator allocator = {};
		for (size_t i = 0; i < framesInFlight; i++)
			m_Allocations[i] = allocator.AllocateBuffer((VkDeviceSize)dataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_Buffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		auto buffers = m_Buffers;
		auto allocations = m_Allocations;

		Renderer::SubmitFree([buffers, allocations]()
		{
			for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
			{
				VulkanAllocator allocator = {};

				if (buffers[i] != VK_NULL_HANDLE)
					allocator.DestroyBuffer(buffers[i], allocations[i]);
			}
		});
	}

	void VulkanStorageBuffer::SetData(void* data, size_t size, size_t offset)
	{
		APP_PROFILE_SCOPE("VulkanStorageBuffer::SetData");

		if (size + offset > m_Size)
		{
			APP_ASSERT(false, "Data exceeds buffer size in SetData()");
			return;
		}

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			void* mappedMemory = nullptr;
			VulkanAllocator::MapMemory(m_Allocations[i], mappedMemory);
			memcpy(static_cast<uint8_t*>(mappedMemory) + offset, data, size);
			VulkanAllocator::UnMapMemory(m_Allocations[i]);
		}
	}

	void* VulkanStorageBuffer::StartRetrieval()
	{
		void* mappedMemory = nullptr;
		VulkanAllocator::MapMemory(m_Allocations[Renderer::GetCurrentFrame()], mappedMemory);

		return mappedMemory;
	}

	void VulkanStorageBuffer::EndRetrieval()
	{
		VulkanAllocator::UnMapMemory(m_Allocations[Renderer::GetCurrentFrame()]);
	}

	void VulkanStorageBuffer::Upload(Ref<DescriptorSet> set, Descriptor element)
	{
		APP_PROFILE_SCOPE("VulkanStorageBuffer::Upload");

		auto vkSet = RefHelper::RefAs<VulkanDescriptorSet>(set);

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = m_Buffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = m_Size;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = vkSet->GetVulkanSet((uint32_t)i);
			descriptorWrite.dstBinding = element.Binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrite.descriptorCount = element.Count;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice(), 1, &descriptorWrite, 0, nullptr);
		}
	}

}