#pragma once

#include <mutex>
#include <vector>

#include <vulkan/vulkan.h>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/RenderInstance.hpp"

#include "Swift/Vulkan/VulkanDevice.hpp"
#include "Swift/Vulkan/VulkanPhysicalDevice.hpp"
#include "Swift/Vulkan/VulkanSwapChain.hpp"

namespace Swift
{

	#ifndef LV_DIST
		inline static constexpr const bool s_Validation = true;
	#else
		inline static constexpr const bool s_Validation = false;
	#endif

	static const std::vector<const char*> s_RequestedValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	static const std::vector<const char*> s_RequestedDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	class VulkanRenderer : public RenderInstance
	{
	public:
		VulkanRenderer();
		virtual ~VulkanRenderer();

		void Init() override;

		void BeginFrame() override;
		void EndFrame() override;

		void Submit(RenderFunction function) override;
		void SubmitFree(FreeFunction function) override;

		void Wait() override;

		void Draw(Ref<CommandBuffer> commandBuffer, uint32_t verticeCount) override;
		void DrawIndexed(Ref<CommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer) override;

		void OnResize(uint32_t width, uint32_t height) override;

		inline Utils::Queue<RenderFunction>& GetRenderQueue() override { return m_RenderQueue; }
		inline Utils::Queue<FreeFunction>& GetFreeQueue() override { return m_ResourceFreeQueue; }

		inline uint32_t GetCurrentFrame() const override { return m_SwapChain->GetCurrentFrame(); }
		inline std::vector<Ref<Image2D>>& GetSwapChainImages() { return m_SwapChain->GetSwapChainImages(); }
		inline Ref<Image2D> GetDepthImage() { return m_SwapChain->GetDepthImage(); }

	public:
		inline VkInstance& GetVulkanInstance() { return m_VulkanInstance; }
		inline VkSurfaceKHR& GetVulkanSurface() { return m_Surface; }

		inline Ref<VulkanDevice> GetLogicalDevice() { return m_Device; }
		inline Ref<VulkanPhysicalDevice> GetPhysicalDevice() { return m_PhysicalDevice; }
		inline Ref<VulkanSwapChain> GetSwapChain() { return m_SwapChain; }

	private:
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		Ref<VulkanPhysicalDevice> m_PhysicalDevice = VK_NULL_HANDLE;
		Ref<VulkanDevice> m_Device = VK_NULL_HANDLE;
		Ref<VulkanSwapChain> m_SwapChain = VK_NULL_HANDLE;

	private:
		Utils::Queue<RenderFunction> m_RenderQueue = { };
		Utils::Queue<FreeFunction> m_ResourceFreeQueue = { };
	};

}