#pragma once

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/Image.hpp"
#include "Swift/Renderer/Pipeline.hpp"
#include "Swift/Renderer/Descriptors.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Swift
{

	VkFormat GetVulkanFormatFromImageFormat(ImageFormat format);
	ImageFormat GetImageFormatFromVulkanFormat(VkFormat format);

	struct VulkanImageData
	{
	public:
		VkImage Image = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VkSampler Sampler = VK_NULL_HANDLE;

	public:
		VulkanImageData() = default;
		VulkanImageData(VkImage image, VkImageView view, VkSampler sampler = VK_NULL_HANDLE);
		VulkanImageData(VkImage image, VmaAllocation allocation, VkImageView view, VkSampler sampler = VK_NULL_HANDLE);
		virtual ~VulkanImageData() = default;
	};

	class VulkanImage2D : public Image2D
	{
	public:
		VulkanImage2D(const ImageSpecification& specs);
		VulkanImage2D(const ImageSpecification& specs, const VulkanImageData& data);
		virtual ~VulkanImage2D();

		void SetData(void* data, size_t size) override;

		void Resize(uint32_t width, uint32_t height) override;

		void Upload(Ref<DescriptorSet> set, Descriptor element) override;
		void Transition(ImageLayout initial, ImageLayout final) override;

		// Helper function for swapchain
		void SetImageData(const ImageSpecification& specs, const VulkanImageData& data);

		inline ImageSpecification& GetSpecification() override { return m_Specification; }
		inline uint32_t GetWidth() const override { return m_Specification.Width; }
		inline uint32_t GetHeight() const override { return m_Specification.Height; }

		VkFormat GetFormat() const;

		inline VkImage GetVulkanImage() { return m_Data.Image; }
		inline VmaAllocation GetAllocation() { return m_Data.Allocation; }
		inline VkImageView GetImageView() { return m_Data.ImageView; }
		inline VkSampler GetSampler() { return m_Data.Sampler; }

		inline VulkanImageData GetImageData() { return m_Data; }

	private:
		void CreateImage(uint32_t width, uint32_t height);
		void CreateImage(const std::filesystem::path& path);

		void GenerateMipmaps(VkImage& image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	private:
		ImageSpecification m_Specification = {};

		VulkanImageData m_Data = {};

		uint32_t m_Miplevels = 1;
	};

}