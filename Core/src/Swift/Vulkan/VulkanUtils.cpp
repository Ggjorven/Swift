#include "swpch.h"
#include "VulkanUtils.hpp"

#include "Swift/Core/Core.hpp"
#include "Swift/Core/Logging.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanRenderer.hpp"

namespace Swift
{

	static VmaAllocator s_Allocator = VK_NULL_HANDLE;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Commands
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VulkanCommand::VulkanCommand(bool start)
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = renderer->GetSwapChain()->GetCommandPool();
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(renderer->GetLogicalDevice()->GetVulkanDevice(), &allocInfo, &m_CommandBuffer))
			APP_LOG_WARN("Failed to create single time command buffer!");

		if (start)
			Begin();
	}

	VulkanCommand::~VulkanCommand()
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();
		vkFreeCommandBuffers(renderer->GetLogicalDevice()->GetVulkanDevice(), renderer->GetSwapChain()->GetCommandPool(), 1, &m_CommandBuffer);
	}

	void VulkanCommand::Begin()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
	}

	void VulkanCommand::End()
	{
		vkEndCommandBuffer(m_CommandBuffer);
	}

	void VulkanCommand::Submit()
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;

		vkQueueSubmit(renderer->GetLogicalDevice()->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(renderer->GetLogicalDevice()->GetGraphicsQueue());
	}

	void VulkanCommand::EndAndSubmit()
	{
		End();
		Submit();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocator
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VmaAllocation VulkanAllocator::AllocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& dstBuffer, VkMemoryPropertyFlags requiredFlags)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Change if necessary

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage; // VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, etc.
		allocInfo.requiredFlags = requiredFlags;

		VmaAllocation allocation = VK_NULL_HANDLE;
		if (vmaCreateBuffer(s_Allocator, &bufferInfo, &allocInfo, &dstBuffer, &allocation, nullptr) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to allocate buffer.");

		return allocation;
	}

	void VulkanAllocator::CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size)
	{
		VulkanCommand command = VulkanCommand(true);

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(command.GetVulkanCommandBuffer(), srcBuffer, dstBuffer, 1, &copyRegion);

		command.EndAndSubmit();
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		vmaDestroyBuffer(s_Allocator, buffer, allocation);
	}

	VmaAllocation VulkanAllocator::AllocateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VkImage& image, VkMemoryPropertyFlags requiredFlags)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = memUsage;
		allocCreateInfo.requiredFlags = requiredFlags;

		VmaAllocation allocation = VK_NULL_HANDLE;
		VkResult result = vmaCreateImage(s_Allocator, &imageInfo, &allocCreateInfo, &image, &allocation, nullptr);
		if (result != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create Vulkan image. Code: {0}", VkResultToString(result));

		return allocation;
	}

	void VulkanAllocator::CopyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height)
	{
		VulkanCommand command = VulkanCommand(true);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(command.GetVulkanCommandBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		command.EndAndSubmit();
	}

	VkImageView VulkanAllocator::CreateImageView(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = aspectFlags;

		VkImageView imageView = VK_NULL_HANDLE;
		if (vkCreateImageView(((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create texture image view!");

		return imageView;
	}

	VkSampler VulkanAllocator::CreateSampler(uint32_t mipLevels)
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(renderer->GetPhysicalDevice()->GetVulkanPhysicalDevice(), &properties);

		samplerInfo.anisotropyEnable = VK_TRUE;								// Can be disabled: just set VK_FALSE
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // And 1.0f

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = (float)mipLevels;
		samplerInfo.mipLodBias = 0.0f; // Optional

		VkSampler sampler = VK_NULL_HANDLE;
		if (vkCreateSampler(renderer->GetLogicalDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create texture sampler!");

		return sampler;
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		vmaDestroyImage(s_Allocator, image, allocation);
	}

	void VulkanAllocator::MapMemory(VmaAllocation& allocation, void*& mapData)
	{
		vmaMapMemory(s_Allocator, allocation, &mapData);
	}

	void VulkanAllocator::UnMapMemory(VmaAllocation& allocation)
	{
		vmaUnmapMemory(s_Allocator, allocation);
	}

	uint32_t VulkanAllocator::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties = {};
		vkGetPhysicalDeviceMemoryProperties(((VulkanRenderer*)Renderer::GetInstance())->GetPhysicalDevice()->GetVulkanPhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}

		APP_LOG_ERROR("Failed to find suitable memory type!");
		return -1;
	}

	bool VulkanAllocator::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkFormat VulkanAllocator::FindDepthFormat()
	{
		return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
	}

	VkFormat VulkanAllocator::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(((VulkanRenderer*)Renderer::GetInstance())->GetPhysicalDevice()->GetVulkanPhysicalDevice(), format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		APP_LOG_ERROR("Failed to find supported format!");
		return VK_FORMAT_UNDEFINED;
	}

	void VulkanAllocator::TransitionImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
	{
		if (oldLayout == newLayout)
			return;

		VulkanCommand command = VulkanCommand(true);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage = {};
		VkPipelineStageFlags destinationStage = {};

		// New layout checks
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format))
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		// Old layout checks
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) 
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; 
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; 
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; 
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}

		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; 
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
			APP_LOG_ERROR("Unsupported layout transition!");

		vkCmdPipelineBarrier(command.GetVulkanCommandBuffer(), sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		command.EndAndSubmit();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialization
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static size_t s_Allocations = 0;

	static void* VKAPI_PTR VmaAllocFn(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
		//s_Allocations++;
		//APP_LOG_TRACE("Allocated Vulkan Memory: (Size: {0} | Total Allocs: {1})", size, s_Allocations);

		return std::malloc(size);
	}

	static void VKAPI_PTR VmaFreeFn(void* pUserData, void* pMemory)
	{
		//s_Allocations--;
		//APP_LOG_TRACE("Freed Vulkan Memory: (Total Allocs: {0})", s_Allocations);
		
		std::free(pMemory);
	}

	static void* VKAPI_PTR VmaReallocFn(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) 
	{
		return std::realloc(pOriginal, size);
	}

	void VulkanAllocator::Init()
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();

		VkAllocationCallbacks callbacks = {};
		callbacks.pUserData = nullptr;
		callbacks.pfnAllocation = VmaAllocFn;
		callbacks.pfnFree = VmaFreeFn;
		callbacks.pfnReallocation = VmaReallocFn;
		callbacks.pfnInternalAllocation = nullptr;
		callbacks.pfnInternalFree = nullptr;

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.instance = renderer->GetVulkanInstance();
		allocatorInfo.physicalDevice = renderer->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		allocatorInfo.device = renderer->GetLogicalDevice()->GetVulkanDevice();
		allocatorInfo.pAllocationCallbacks = &callbacks;

		if (vmaCreateAllocator(&allocatorInfo, &s_Allocator) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create VMA allocator!");
	}

	void VulkanAllocator::Destroy()
	{
		if (s_Allocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(s_Allocator);
			s_Allocator = VK_NULL_HANDLE;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utils
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::string VkResultToString(VkResult result)
	{
		switch (result)
		{
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT: return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
		}

		return "";
	}

	std::string VkObjectTypeToString(const VkObjectType type) 
	{
		switch (type)
		{
		case VK_OBJECT_TYPE_COMMAND_BUFFER: return "VK_OBJECT_TYPE_COMMAND_BUFFER";
		case VK_OBJECT_TYPE_PIPELINE: return "VK_OBJECT_TYPE_PIPELINE";
		case VK_OBJECT_TYPE_FRAMEBUFFER: return "VK_OBJECT_TYPE_FRAMEBUFFER";
		case VK_OBJECT_TYPE_IMAGE: return "VK_OBJECT_TYPE_IMAGE";
		case VK_OBJECT_TYPE_QUERY_POOL: return "VK_OBJECT_TYPE_QUERY_POOL";
		case VK_OBJECT_TYPE_RENDER_PASS: return "VK_OBJECT_TYPE_RENDER_PASS";
		case VK_OBJECT_TYPE_COMMAND_POOL: return "VK_OBJECT_TYPE_COMMAND_POOL";
		case VK_OBJECT_TYPE_PIPELINE_CACHE: return "VK_OBJECT_TYPE_PIPELINE_CACHE";
		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR: return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR";
		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV: return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV";
		case VK_OBJECT_TYPE_BUFFER: return "VK_OBJECT_TYPE_BUFFER";
		case VK_OBJECT_TYPE_BUFFER_VIEW: return "VK_OBJECT_TYPE_BUFFER_VIEW";
		case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT: return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
		case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
		case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR: return "VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR";
		case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
		case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE: return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
		case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT: return "VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT";
		case VK_OBJECT_TYPE_DEVICE: return "VK_OBJECT_TYPE_DEVICE";
		case VK_OBJECT_TYPE_DEVICE_MEMORY: return "VK_OBJECT_TYPE_DEVICE_MEMORY";
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT: return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
		case VK_OBJECT_TYPE_DISPLAY_KHR: return "VK_OBJECT_TYPE_DISPLAY_KHR";
		case VK_OBJECT_TYPE_DISPLAY_MODE_KHR: return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
		case VK_OBJECT_TYPE_PHYSICAL_DEVICE: return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
		case VK_OBJECT_TYPE_EVENT: return "VK_OBJECT_TYPE_EVENT";
		case VK_OBJECT_TYPE_FENCE: return "VK_OBJECT_TYPE_FENCE";
		case VK_OBJECT_TYPE_IMAGE_VIEW: return "VK_OBJECT_TYPE_IMAGE_VIEW";
		case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV: return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV";
		case VK_OBJECT_TYPE_INSTANCE: return "VK_OBJECT_TYPE_INSTANCE";
		case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL: return "VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL";
		case VK_OBJECT_TYPE_QUEUE: return "VK_OBJECT_TYPE_QUEUE";
		case VK_OBJECT_TYPE_SAMPLER: return "VK_OBJECT_TYPE_SAMPLER";
		case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION: return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
		case VK_OBJECT_TYPE_SEMAPHORE: return "VK_OBJECT_TYPE_SEMAPHORE";
		case VK_OBJECT_TYPE_SHADER_MODULE: return "VK_OBJECT_TYPE_SHADER_MODULE";
		case VK_OBJECT_TYPE_SURFACE_KHR: return "VK_OBJECT_TYPE_SURFACE_KHR";
		case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
		case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT: return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
		case VK_OBJECT_TYPE_UNKNOWN: return "VK_OBJECT_TYPE_UNKNOWN";
		case VK_OBJECT_TYPE_MAX_ENUM: return "VK_OBJECT_TYPE_MAX_ENUM";
		}

		return "";
	}

}