#include "swpch.h"
#include "VulkanPhysicalDevice.hpp"

#include "Swift/Core/Logging.hpp"
#include "Swift/Core/Application.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanRenderer.hpp"

namespace Swift
{
	VulkanPhysicalDevice::VulkanPhysicalDevice()
	{
		VkInstance& instance = ((VulkanRenderer*)Renderer::GetInstance())->GetVulkanInstance();

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
			APP_LOG_ERROR("Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (PhysicalDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				break;
			}
		}

		m_Depthformat = GetDepthFormat();

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);

		// Note(Jorben): Check if no device was selected
		APP_VERIFY(m_PhysicalDevice, "Verify failed: Failed to find suitable GPU");
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice()
	{
		// Note(Jorben): Since a Physical Device is not something we created there is nothing to destroy
	}

	QueueFamilyIndices QueueFamilyIndices::Find(const VkPhysicalDevice& device)
	{
		QueueFamilyIndices indices = {};

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int32_t i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			// Early exit check
			if (indices.IsComplete())
				break;

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.GraphicsFamily = i;

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
				indices.ComputeFamily = i;

			VkBool32 presentSupport = false;
			VkSurfaceKHR& surface = ((VulkanRenderer*)Renderer::GetInstance())->GetVulkanSurface();
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
				indices.PresentFamily = i;

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails SwapChainSupportDetails::Query(const VkPhysicalDevice& device)
	{
		SwapChainSupportDetails details;

		VkSurfaceKHR& surface = ((VulkanRenderer*)Renderer::GetInstance())->GetVulkanSurface();

		// Capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

		// Formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.Formats.resize((size_t)formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
		}

		// Presentation modes
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.PresentModes.resize((size_t)presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	bool VulkanPhysicalDevice::PhysicalDeviceSuitable(const VkPhysicalDevice& device)
	{
		QueueFamilyIndices indices = QueueFamilyIndices::Find(device);

		bool extensionsSupported = ExtensionsSupported(device);
		bool swapChainAdequate = false;

		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = SwapChainSupportDetails::Query(device);
			swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.fillModeNonSolid && supportedFeatures.wideLines;
	}

	bool VulkanPhysicalDevice::ExtensionsSupported(const VkPhysicalDevice& device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(s_RequestedDeviceExtensions.begin(), s_RequestedDeviceExtensions.end());

		for (const auto& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);

		// Note(Jorben): It's empty if all the required extensions are available
		return requiredExtensions.empty();
	}

	VkFormat VulkanPhysicalDevice::GetDepthFormat()
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		std::vector<VkFormat> depthFormats =
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProps);
			// Format must support depth stencil attachment for optimal tiling
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				return format;
		}

		return VK_FORMAT_UNDEFINED;
	}

	Ref<VulkanPhysicalDevice> VulkanPhysicalDevice::Select()
	{
		return RefHelper::Create<VulkanPhysicalDevice>();
	}

}