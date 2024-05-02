#include "swpch.h"
#include "VulkanImage.hpp"

#include "Swift/Core/Logging.hpp"
#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanPipeline.hpp"
#include "Swift/Vulkan/VulkanDescriptors.hpp"

#include <stb_image.h>

namespace Swift
{

	static VkImageUsageFlags GetVulkanImageUsageFromImageUsage(ImageUsageFlags usage);
	static VkImageAspectFlags GetVulkanImageAspectFromImageUsage(ImageUsageFlags usage);

	VulkanImage2D::VulkanImage2D(const ImageSpecification& specs)
		: m_Specification(specs)
	{
		if (!(m_Specification.Flags & ImageUsageFlags::Colour) && !(m_Specification.Flags & ImageUsageFlags::Depth))
		{
			APP_ASSERT(false, "Tried to create image without specifying if it's a Colour or Depth image.");
			return;
		}

		switch (m_Specification.Usage)
		{
		case ImageUsage::Size:
			CreateImage(m_Specification.Width, m_Specification.Height);
			break;
		case ImageUsage::File:
			CreateImage(m_Specification.Path);
			break;

		default:
			APP_LOG_ERROR("Invalid image usage selected.");
			break;
		}
	}

	VulkanImage2D::VulkanImage2D(const ImageSpecification& specs, const VulkanImageData& data)
	{
		SetImageData(specs, data);
	}

	VulkanImage2D::~VulkanImage2D()
	{
		auto data = m_Data;

		Renderer::SubmitFree([data]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();
			Renderer::Wait();

			vkDestroySampler(device, data.Sampler, nullptr);
			vkDestroyImageView(device, data.ImageView, nullptr);

			VulkanAllocator allocator = {};
			if (data.Image != VK_NULL_HANDLE && data.Allocation != VK_NULL_HANDLE)
				allocator.DestroyImage(data.Image, data.Allocation);
		});
	}

	void VulkanImage2D::SetData(void* data, size_t size)
	{
		APP_PROFILE_SCOPE("VulkanImage2D::SetData");

		VulkanAllocator allocator = {};

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
		stagingBufferAllocation = allocator.AllocateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

		void* mappedData;
		VulkanAllocator::MapMemory(stagingBufferAllocation, mappedData);
		memcpy(mappedData, data, size);
		VulkanAllocator::UnMapMemory(stagingBufferAllocation);

		VulkanAllocator::TransitionImageLayout(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_Miplevels);
		allocator.CopyBufferToImage(stagingBuffer, m_Data.Image, m_Specification.Width, m_Specification.Height);

		if (m_Specification.Flags & ImageUsageFlags::NoMipMaps)
		{
			VulkanAllocator::TransitionImageLayout(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (VkImageLayout)m_Specification.Layout, m_Miplevels);
		}
		else
		{
			GenerateMipmaps(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), m_Specification.Width, m_Specification.Height, m_Miplevels);
			VulkanAllocator::TransitionImageLayout(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (VkImageLayout)m_Specification.Layout, m_Miplevels);
		}

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

	void VulkanImage2D::Resize(uint32_t width, uint32_t height)
	{
		auto data = m_Data;

		Renderer::SubmitFree([data]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

			vkDestroySampler(device, data.Sampler, nullptr);
			vkDestroyImageView(device, data.ImageView, nullptr);

			VulkanAllocator allocator = {};
			if (data.Image != VK_NULL_HANDLE && data.Allocation != VK_NULL_HANDLE)
				allocator.DestroyImage(data.Image, data.Allocation);
		});

		CreateImage(width, height);
	}

	void VulkanImage2D::Upload(Ref<DescriptorSet> set, Descriptor element)
	{
		APP_PROFILE_SCOPE("VulkanImage2D::Upload");

		auto vkSet = RefHelper::RefAs<VulkanDescriptorSet>(set);

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = (VkImageLayout)m_Specification.Layout;
			imageInfo.imageView = m_Data.ImageView;
			imageInfo.sampler = m_Data.Sampler;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = vkSet->GetVulkanSet((uint32_t)i);
			descriptorWrite.dstBinding = element.Binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = DescriptorTypeToVulkanDescriptorType(element.Type);
			descriptorWrite.descriptorCount = element.Count;
			descriptorWrite.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice(), 1, &descriptorWrite, 0, nullptr);
		}
	}

	void VulkanImage2D::Transition(ImageLayout initial, ImageLayout final)
	{
		VulkanAllocator::TransitionImageLayout(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), (VkImageLayout)initial, (VkImageLayout)final, m_Miplevels);
		m_Specification.Layout = final;
	}

	void VulkanImage2D::SetImageData(const ImageSpecification& specs, const VulkanImageData& data)
	{
		m_Specification = specs;
		m_Data = data;
	}

	VkFormat VulkanImage2D::GetFormat() const
	{
		return GetVulkanFormatFromImageFormat(m_Specification.Format);
	}

	void VulkanImage2D::CreateImage(uint32_t width, uint32_t height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		if (!(m_Specification.Flags & ImageUsageFlags::NoMipMaps))
			m_Miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		VulkanAllocator allocator = {};
		m_Data.Allocation = allocator.AllocateImage(width, height, m_Miplevels, GetVulkanFormatFromImageFormat(m_Specification.Format), VK_IMAGE_TILING_OPTIMAL, GetVulkanImageUsageFromImageUsage(m_Specification.Flags), VMA_MEMORY_USAGE_GPU_ONLY, m_Data.Image);

		m_Data.ImageView = allocator.CreateImageView(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), GetVulkanImageAspectFromImageUsage(m_Specification.Flags), m_Miplevels);
		m_Data.Sampler = allocator.CreateSampler(m_Miplevels);

		VulkanAllocator::TransitionImageLayout(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), VK_IMAGE_LAYOUT_UNDEFINED, (VkImageLayout)m_Specification.Layout, m_Miplevels);
	}

	void VulkanImage2D::CreateImage(const std::filesystem::path& path)
	{
		int width, height, texChannels;
		stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
		if (pixels == nullptr)
		{
			APP_ASSERT(false, "Failed to load image from '{0}'", path.string());
			return;
		}

		m_Specification.Width = width;
		m_Specification.Height = height;
		if (!(m_Specification.Flags & ImageUsageFlags::NoMipMaps))
			m_Miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		m_Specification.Format = ImageFormat::RGBA;
		size_t imageSize = m_Specification.Width * m_Specification.Height * 4;

		VulkanAllocator allocator = {};
		m_Data.Allocation = allocator.AllocateImage(m_Specification.Width, m_Specification.Height, m_Miplevels, GetVulkanFormatFromImageFormat(m_Specification.Format), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | GetVulkanImageUsageFromImageUsage(m_Specification.Flags), VMA_MEMORY_USAGE_GPU_ONLY, m_Data.Image);

		m_Data.ImageView = allocator.CreateImageView(m_Data.Image, GetVulkanFormatFromImageFormat(m_Specification.Format), VK_IMAGE_ASPECT_COLOR_BIT, m_Miplevels);
		m_Data.Sampler = allocator.CreateSampler(m_Miplevels);

		SetData((void*)pixels, imageSize);
		stbi_image_free((void*)pixels);
	}

	void VulkanImage2D::GenerateMipmaps(VkImage& image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(((VulkanRenderer*)Renderer::GetInstance())->GetPhysicalDevice()->GetVulkanPhysicalDevice(), imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
			APP_LOG_ERROR("Texture image format does not support linear blitting!");

		VulkanCommand command = VulkanCommand(true);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(command.GetVulkanCommandBuffer(),
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(command.GetVulkanCommandBuffer(),
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(command.GetVulkanCommandBuffer(),
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command.GetVulkanCommandBuffer(),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		command.EndAndSubmit();
	}

	VkFormat GetVulkanFormatFromImageFormat(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGBA:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case ImageFormat::sRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case ImageFormat::BGRA:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case ImageFormat::Depth32SFloat:
			return VK_FORMAT_D32_SFLOAT;
		case ImageFormat::Depth32SFloatS8:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case ImageFormat::Depth24UnormS8:
			return VK_FORMAT_D24_UNORM_S8_UINT;

		default:
			APP_LOG_ERROR("Invalid image format.");
			break;
		}

		return VK_FORMAT_UNDEFINED;
	}
	
	ImageFormat GetImageFormatFromVulkanFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8G8B8A8_UNORM:
			return ImageFormat::RGBA;
		case VK_FORMAT_R8G8B8A8_SRGB:
			return ImageFormat::sRGB;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return ImageFormat::BGRA;
		case VK_FORMAT_D32_SFLOAT:
			return ImageFormat::Depth32SFloat;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return ImageFormat::Depth32SFloatS8;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return ImageFormat::Depth24UnormS8;

		default:
			APP_LOG_ERROR("Invalid image format.");
			break;
		}

		return ImageFormat::None;
	}

	static VkImageUsageFlags GetVulkanImageUsageFromImageUsage(ImageUsageFlags usage)
	{
		VkImageUsageFlags flags = 0;

		if (usage & ImageUsageFlags::Colour)
			flags = flags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (usage & ImageUsageFlags::Depth)
			flags = flags | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (usage & ImageUsageFlags::Sampled)
			flags = flags | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (usage & ImageUsageFlags::Storage)
			flags = flags | VK_IMAGE_USAGE_STORAGE_BIT;
		if (usage & ImageUsageFlags::Transient)
			flags = flags | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		if (usage & ImageUsageFlags::Input)
			flags = flags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

		return flags;
	}

	static VkImageAspectFlags GetVulkanImageAspectFromImageUsage(ImageUsageFlags usage)
	{
		VkImageAspectFlags flags = 0;
		
		if (usage & ImageUsageFlags::Colour)
			flags = flags | VK_IMAGE_ASPECT_COLOR_BIT;
		if (usage & ImageUsageFlags::Depth)
			flags = flags | VK_IMAGE_ASPECT_DEPTH_BIT;

		return flags;
	}



	VulkanImageData::VulkanImageData(VkImage image, VkImageView view, VkSampler sampler)
		: Image(image), ImageView(view), Sampler(sampler)
	{
	}

	VulkanImageData::VulkanImageData(VkImage image, VmaAllocation allocation, VkImageView view, VkSampler sampler)
		: Image(image), Allocation(allocation), ImageView(view), Sampler(sampler)
	{
	}

}