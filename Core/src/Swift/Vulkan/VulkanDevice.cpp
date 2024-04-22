#include "swpch.h"
#include "VulkanDevice.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Vulkan/VulkanRenderer.hpp"

namespace Swift
{

	VulkanDevice::VulkanDevice(Ref<VulkanPhysicalDevice> physicalDevice)
		: m_PhysicalDevice(physicalDevice)
	{
		QueueFamilyIndices indices = QueueFamilyIndices::Find(m_PhysicalDevice->GetVulkanPhysicalDevice());

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.wideLines = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(s_RequestedDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = s_RequestedDeviceExtensions.data();

		if constexpr (s_Validation)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(s_RequestedValidationLayers.size());
			createInfo.ppEnabledLayerNames = s_RequestedValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create logical device!");

		// Retrieve the graphics/compute/present queue handle
		vkGetDeviceQueue(m_LogicalDevice, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, indices.ComputeFamily.value(), 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_LogicalDevice, indices.PresentFamily.value(), 0, &m_PresentQueue);
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	Ref<VulkanDevice> VulkanDevice::Create(Ref<VulkanPhysicalDevice> physicalDevice)
	{
		return RefHelper::Create<VulkanDevice>(physicalDevice);
	}

}