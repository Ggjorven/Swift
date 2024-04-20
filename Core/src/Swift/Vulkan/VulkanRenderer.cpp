#include "swpch.h"
#include "VulkanRenderer.hpp"

#include "Swift/Core/Application.hpp"
#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		APP_LOG_WARN("Validation layer: {0}", pCallbackData->pMessage);
		return VK_FALSE;
	}

	return VK_FALSE;
}

namespace Swift
{

	VulkanRenderer::VulkanRenderer()
	{
		// TODO: Initialize
	}

	VulkanRenderer::~VulkanRenderer()
	{
		// TODO: Add more

		/*
		if constexpr (s_Validation)
			DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

		vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
		*/
	}

	void VulkanRenderer::BeginFrame()
	{
		if (Application::Get().IsMinimized())
			return;

		Renderer::GetRenderData().Reset();
		m_ResourceFreeQueue.Execute();
	}

	void VulkanRenderer::EndFrame() // A.k.a Display/Present
	{
		if (Application::Get().IsMinimized())
			return;
	}

	void VulkanRenderer::Submit(RenderFunction function)
	{
		m_RenderQueue.Add(function);
	}

	void VulkanRenderer::SubmitFree(FreeFunction function)
	{
		m_ResourceFreeQueue.Add(function);
	}

	void VulkanRenderer::Wait()
	{
	}

	/*
	void VulkanRenderer::DrawIndexed(Ref<RenderCommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer)
	{
		LV_PROFILE_SCOPE("VulkanRenderer::DrawIndexed");
		s_RenderData->DrawCalls++;

		auto cmdBuf = RefHelper::RefAs<VulkanRenderCommandBuffer>(commandBuffer);
		vkCmdDrawIndexed(cmdBuf->GetVulkanCommandBuffer(), indexBuffer->GetCount(), 1, 0, 0, 0);
	}
	*/

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
	}

}