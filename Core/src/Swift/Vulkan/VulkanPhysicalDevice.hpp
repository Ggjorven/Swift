#pragma once

#include <stdint.h>
#include <memory>

#include <vulkan/vulkan.h>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

namespace Swift
{

	struct QueueFamilyIndices
	{
	public:
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> ComputeFamily;
		std::optional<uint32_t> PresentFamily;

		static QueueFamilyIndices Find(const VkPhysicalDevice& device);

	public:
		inline bool IsComplete() const { return GraphicsFamily.has_value() && ComputeFamily.has_value() && PresentFamily.has_value(); }
	};

	struct SwapChainSupportDetails
	{
	public:
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		static SwapChainSupportDetails Query(const VkPhysicalDevice& device);
	};

	class VulkanPhysicalDevice
	{
	public:
		VulkanPhysicalDevice();
		virtual ~VulkanPhysicalDevice();

		inline VkPhysicalDevice& GetVulkanPhysicalDevice() { return m_PhysicalDevice; }

		inline VkFormat GetDepthFormat() const { return m_Depthformat; }
		inline const VkPhysicalDeviceProperties& GetProperties() { return m_Properties; }

		static Ref<VulkanPhysicalDevice> Select();

	private:
		bool PhysicalDeviceSuitable(const VkPhysicalDevice& device);
		bool ExtensionsSupported(const VkPhysicalDevice& device);
		VkFormat GetDepthFormat();

	private:
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		
		VkPhysicalDeviceProperties m_Properties = {};
		VkFormat m_Depthformat = VK_FORMAT_UNDEFINED;
	};

}