#include "swpch.h"
#include "VulkanSwapChain.hpp"

#include "Swift/Core/Logging.hpp"
#include "Swift/Core/Application.hpp"

#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanImage.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanTaskManager.hpp"

namespace Swift
{

	VulkanSwapChain::VulkanSwapChain(VkInstance vkInstance, Ref<VulkanDevice> vkDevice)
		: m_Instance(vkInstance), m_Device(vkDevice)
	{
		FindImageFormatAndColorSpace();
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		Renderer::Wait();
		auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

		if (m_SwapChain)
			vkDestroySwapchainKHR(device, m_SwapChain, nullptr);

		m_Images.clear();
		m_DepthStencil.reset();

		vkDestroyCommandPool(device, m_CommandPool, nullptr);

		for (size_t i = 0; i < (size_t)RendererSpecification::BufferCount; i++)
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
	}

	void VulkanSwapChain::BeginFrame()
	{
		m_AquiredImage = AcquireNextImage();
	}

	void VulkanSwapChain::EndFrame()
	{
		auto& semaphores = VulkanTaskManager::GetSemaphores();

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = (uint32_t)semaphores.size();
		presentInfo.pWaitSemaphores = semaphores.data();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain;
		presentInfo.pImageIndices = &m_AquiredImage;
		presentInfo.pResults = nullptr; // Optional

		VkResult result = VK_SUCCESS;
		{
			APP_PROFILE_SCOPE("QueuePresent");

			// Note(Jorben): Without these 2 lines there is a memory leak when validation layers are enabled.
			if constexpr (s_Validation)
			{
				APP_PROFILE_SCOPE("QueueWaitIdle");
				vkQueueWaitIdle(m_Device->GetGraphicsQueue());
			}
			
			result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);
		}

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			auto& window = Application::Get().GetWindow();
			OnResize(window.GetWidth(), window.GetHeight(), window.IsVSync());
		}
		else if (result != VK_SUCCESS)
		{
			APP_LOG_ERROR("Failed to present swap chain image!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % (uint32_t)RendererSpecification::BufferCount;
	}

	void VulkanSwapChain::OnResize(uint32_t width, uint32_t height, const bool vsync)
	{
		auto device = m_Device->GetVulkanDevice();

		Init(width, height, vsync);
	}

	void VulkanSwapChain::Init(uint32_t width, uint32_t height, const bool vsync)
	{
		VkDevice device = m_Device->GetVulkanDevice();
		VkPhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		VkSurfaceKHR& surface = ((VulkanRenderer*)Renderer::GetInstance())->GetVulkanSurface();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Command pools
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		uint32_t framesInFlight = (uint32_t)RendererSpecification::BufferCount;
		if (!m_CommandPool)
		{
			QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices::Find(m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice());

			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

			if (vkCreateCommandPool(m_Device->GetVulkanDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
				APP_LOG_ERROR("Failed to create command pool!");
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// SwapChain 
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		SwapChainSupportDetails details = SwapChainSupportDetails::Query(physicalDevice);

		VkExtent2D swapchainExtent = {};
		// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
		if (details.Capabilities.currentExtent.width == (uint32_t)-1)
		{
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			swapchainExtent.width = width;
			swapchainExtent.height = height;
		}
		else
		{
			// If the surface size is defined, the swap chain size must match
			swapchainExtent = details.Capabilities.currentExtent;
		}

		if (width == 0 || height == 0)
			return;

		// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
		// This mode waits for the vertical blank ("v-sync")
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

		// If v-sync is not requested, try to find a mailbox mode
		// It's the lowest latency non-tearing present mode available
		if (!vsync)
		{
			for (size_t i = 0; i < details.PresentModes.size(); i++)
			{
				if (details.PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (details.PresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
				{
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}

		// Determine the number of images
		uint32_t desiredNumberOfSwapchainImages = details.Capabilities.minImageCount + 1;
		if ((details.Capabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > details.Capabilities.maxImageCount))
		{
			desiredNumberOfSwapchainImages = details.Capabilities.maxImageCount;
		}

		// Find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if (details.Capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		else
			preTransform = details.Capabilities.currentTransform;

		// Find a supported composite alpha format (not all devices support alpha opaque)
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// Simply select the first composite alpha format available
		std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {

			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		for (auto& compositeAlphaFlag : compositeAlphaFlags)
		{
			if (details.Capabilities.supportedCompositeAlpha & compositeAlphaFlag)
			{
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.pNext = NULL;
		swapchainCI.surface = surface;
		swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCI.imageFormat = m_ColourFormat;
		swapchainCI.imageColorSpace = m_ColourSpace;
		swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };

		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (details.Capabilities.supportedTransforms & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (details.Capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.oldSwapchain = m_SwapChain;
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = compositeAlpha;

		// Enable transfer source on swap chain images if supported
		if (details.Capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		// Enable transfer destination on swap chain images if supported
		if (details.Capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		auto oldSwapchain = m_SwapChain;
		vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &m_SwapChain);

		if (oldSwapchain)
			vkDestroySwapchainKHR(device, oldSwapchain, nullptr); // Destroys Images?

		// Destroy old swapchain image(view)s
		if (Renderer::Initialized())
		{
			std::vector<VulkanImageData> images = { };
			for (auto& image : m_Images)
			{
				auto vkImage = RefHelper::RefAs<VulkanImage2D>(image);
				images.push_back(vkImage->GetImageData());
			}

			Renderer::SubmitFree([device, images]()
			{
				for (auto& image : images)
					vkDestroyImageView(device, image.ImageView, nullptr);
			});
		}

		// Get the swap chain images
		uint32_t imageCount = 0;
		std::vector<VkImage> tempImages = { };

		vkGetSwapchainImagesKHR(device, m_SwapChain, &imageCount, NULL);
		tempImages.resize(imageCount);
		if (m_Images.empty()) m_Images.resize((size_t)imageCount);
		vkGetSwapchainImagesKHR(device, m_SwapChain, &imageCount, tempImages.data());

		for (uint32_t i = 0; i < imageCount; i++)
		{
			VulkanImageData data = {};
			data.Image = tempImages[i];
			VulkanAllocator::TransitionImageLayout(tempImages[i], m_ColourFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

			VkImageViewCreateInfo colorAttachmentView = {};
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = NULL;
			colorAttachmentView.format = m_ColourFormat;
			colorAttachmentView.image = tempImages[i];
			colorAttachmentView.components =
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			};
			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;

			if (vkCreateImageView(device, &colorAttachmentView, nullptr, &data.ImageView) != VK_SUCCESS)
				APP_LOG_FATAL("Failed to create swapchain imageviews.");

			ImageSpecification specs = {};
			specs.Usage = ImageUsage::None;
			specs.Format = GetImageFormatFromVulkanFormat(m_ColourFormat);
			specs.Flags = ImageUsageFlags::Colour | ImageUsageFlags::NoMipMaps;
			specs.Width = width;
			specs.Height = height;
			specs.Layout = ImageLayout::Presentation;

			if (!m_Images[i]) m_Images[i] = RefHelper::Create<VulkanImage2D>(specs, data);
			else RefHelper::RefAs<VulkanImage2D>(m_Images[i])->SetImageData(specs, data);
		}

		if (!m_DepthStencil)
		{
			ImageSpecification specs = {};
			specs.Usage = ImageUsage::Size;
			specs.Format = GetImageFormatFromVulkanFormat(VulkanAllocator::FindDepthFormat());
			specs.Flags = ImageUsageFlags::Depth | ImageUsageFlags::Sampled /*| ImageUsageFlags::Storage*/ | ImageUsageFlags::NoMipMaps;
			specs.Width = width;
			specs.Height = height;
			specs.Layout = ImageLayout::Depth;

			m_DepthStencil = Image2D::Create(specs);
		}
		else
			m_DepthStencil->Resize(width, height);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Synchronization Objects
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (m_ImageAvailableSemaphores.empty())
		{
			m_ImageAvailableSemaphores.resize(framesInFlight);

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (size_t i = 0; i < framesInFlight; i++)
			{
				if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS)
				{
					APP_LOG_ERROR("Failed to create synchronization objects for a frame!");
				}
			}
		}
	}

	uint32_t VulkanSwapChain::AcquireNextImage()
	{
		APP_PROFILE_SCOPE("AquireNextImage");
		uint32_t imageIndex = 0;

		VkResult result = vkAcquireNextImageKHR(m_Device->GetVulkanDevice(), m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			auto& window = Application::Get().GetWindow();
			OnResize(window.GetWidth(), window.GetHeight(), window.IsVSync());
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			APP_LOG_ERROR("Failed to acquire swap chain image!");
		}

		return imageIndex;
	}

	void VulkanSwapChain::FindImageFormatAndColorSpace()
	{
		VkPhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		VkSurfaceKHR& surface = ((VulkanRenderer*)Renderer::GetInstance())->GetVulkanSurface();

		// Get list of supported surface formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

		// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
		if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			m_ColourFormat = VK_FORMAT_B8G8R8A8_UNORM;
			m_ColourSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			bool found_B8G8R8A8_UNORM = false;
			for (auto&& surfaceFormat : surfaceFormats)
			{
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				{
					m_ColourFormat = surfaceFormat.format;
					m_ColourSpace = surfaceFormat.colorSpace;
					found_B8G8R8A8_UNORM = true;
					break;
				}
			}

			// in case VK_FORMAT_B8G8R8A8_UNORM is not available
			// select the first available color format
			if (!found_B8G8R8A8_UNORM)
			{
				m_ColourFormat = surfaceFormats[0].format;
				m_ColourSpace = surfaceFormats[0].colorSpace;
			}
		}
	}

	Ref<VulkanSwapChain> VulkanSwapChain::Create(VkInstance vkInstance, Ref<VulkanDevice> vkDevice)
	{
		return RefHelper::Create<VulkanSwapChain>(vkInstance, vkDevice);
	}
}