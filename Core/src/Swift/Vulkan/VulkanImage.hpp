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

	class VulkanImage2D : public Image2D
	{
	public:
		VulkanImage2D(const ImageSpecification& specs);
		virtual ~VulkanImage2D();

		void SetData(void* data, size_t size) override;

		void Resize(uint32_t width, uint32_t height) override;

		void Upload(Ref<DescriptorSet> set, Descriptor element) override;
		void Transition(ImageLayout initial, ImageLayout final) override;

		inline ImageSpecification& GetSpecification() override { return m_Specification; }
		inline uint32_t GetWidth() const override { return m_Specification.Width; }
		inline uint32_t GetHeight() const override { return m_Specification.Height; }

		VkFormat GetFormat() const;

		inline VkImage GetVulkanImage() { return m_Image; }
		inline VkImageView GetImageView() { return m_ImageView; }
		inline VkSampler GetSampler() { return m_Sampler; }

	private:
		void CreateImage(uint32_t width, uint32_t height);
		void CreateImage(const std::filesystem::path& path);

		void GenerateMipmaps(VkImage& image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	private:
		ImageSpecification m_Specification = {};

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;

		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;

		uint32_t m_Miplevels = 1;
	};

}