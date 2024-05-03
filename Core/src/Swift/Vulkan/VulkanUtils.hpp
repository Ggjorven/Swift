#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Swift
{

	// Only to be used once
	class VulkanCommand
	{
	public:
		VulkanCommand(bool start = false);
		virtual ~VulkanCommand();

		void Begin();
		void End();
		void Submit();
		void EndAndSubmit();

		inline VkCommandBuffer GetVulkanCommandBuffer() { return m_CommandBuffer; }

	private:
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
	};

	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		virtual ~VulkanAllocator() = default;

		VmaAllocation AllocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& dstBuffer, VkMemoryPropertyFlags requiredFlags = 0);
		void CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size);
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		VmaAllocation AllocateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VkImage& image, VkMemoryPropertyFlags requiredFlags = {});
		void CopyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height);
		VkImageView CreateImageView(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
		VkSampler CreateSampler(uint32_t mipLevels); // TODO: Make it usable with multiple settings.
		void DestroyImage(VkImage image, VmaAllocation allocation);

	public:
		static void MapMemory(VmaAllocation& allocation, void*& mapData);
		static void UnMapMemory(VmaAllocation& allocation);

		static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		static bool HasStencilComponent(VkFormat format);

		static VkFormat FindDepthFormat();
		static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		static void TransitionImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	public:
		static void Init();
		static void Destroy();
	};

	std::string VkResultToString(VkResult result);
	std::string VkObjectTypeToString(const VkObjectType type);

}