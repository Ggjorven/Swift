#include "swpch.h"
#include "VulkanRenderer.hpp"

#include "Swift/Core/Application.hpp"
#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanBuffers.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanTaskManager.hpp"
#include "Swift/Vulkan/VulkanCommandBuffer.hpp"

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
	}

	VulkanRenderer::~VulkanRenderer()
	{
		Wait();

		m_SwapChain->GetSwapChainImages().clear(); // TODO: Find a better way to do this
		m_SwapChain->GetDepthImage().reset(); // TODO: Find a better way to do this
		m_ResourceFreeQueue.Execute();
		
		m_SwapChain.reset();
		VulkanAllocator::Destroy(); 

		m_Device.reset();

		if constexpr (s_Validation)
			DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

		vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
	}

	void VulkanRenderer::Init()
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Instance Creation
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Swift Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.pEngineName = "Swift Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		#if defined(APP_PLATFORM_WINDOWS)
			#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
		#elif defined(APP_PLATFORM_LINUX)
			#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
		#endif

		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
		if constexpr (s_Validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Very little performance hit, can be used in Release.
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		#if defined(LV_PLATFORM_MACOS)
			createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		#endif
		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();

		if constexpr (s_Validation)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(s_RequestedValidationLayers.size());
			createInfo.ppEnabledLayerNames = s_RequestedValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		// Note(Jorben): Setup the debug messenger also for the create instance 
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

		if constexpr (s_Validation)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(s_RequestedValidationLayers.size());
			createInfo.ppEnabledLayerNames = s_RequestedValidationLayers.data();

			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
			APP_LOG_FATAL("Failed to create Vulkan Instance.");

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Debugger Creation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if constexpr (s_Validation)
		{
			if (CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugCreateInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
				APP_LOG_ERROR("Failed to set up debug messenger!");
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Surface Creation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		GLFWwindow* handle = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		if (glfwCreateWindowSurface(m_VulkanInstance, handle, nullptr, &m_Surface) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create window surface!");

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Other
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		m_PhysicalDevice = VulkanPhysicalDevice::Select();
		m_Device = VulkanDevice::Create(m_PhysicalDevice);

		VulkanAllocator::Init();
		auto& window = Application::Get().GetWindow();
		m_SwapChain = VulkanSwapChain::Create(m_VulkanInstance, m_Device);
		m_SwapChain->Init(window.GetWidth(), window.GetHeight(), window.IsVSync());
	}

	void VulkanRenderer::BeginFrame()
	{
		if (Application::Get().IsMinimized())
			return;

		Renderer::GetRenderData().Reset();
		m_ResourceFreeQueue.Execute();

		auto& fences = VulkanTaskManager::GetFences();
		if (!fences.empty())
		{
			vkWaitForFences(m_Device->GetVulkanDevice(), (uint32_t)fences.size(), fences.data(), VK_TRUE, MAX_UINT64);
			vkResetFences(m_Device->GetVulkanDevice(), (uint32_t)fences.size(), fences.data());
		}
		VulkanTaskManager::AddSemaphore(m_SwapChain->GetCurrentImageAvailableSemaphore());

		m_SwapChain->BeginFrame();
	}

	void VulkanRenderer::EndFrame() // A.k.a Display/Present
	{
		if (Application::Get().IsMinimized())
			return;

		{
			APP_PROFILE_SCOPE("RenderQueue");
			m_RenderQueue.Execute();
		}

		m_SwapChain->EndFrame();
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
		vkDeviceWaitIdle(m_Device->GetVulkanDevice());
	}

	void VulkanRenderer::Draw(Ref<CommandBuffer> commandBuffer, uint32_t verticeCount)
	{
		APP_PROFILE_SCOPE("VulkanRenderer::Draw");
		Renderer::GetRenderData().DrawCalls++;

		auto cmdBuf = RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer);
		vkCmdDraw(cmdBuf->GetVulkanCommandBuffer(m_SwapChain->GetCurrentFrame()), verticeCount, 1, 0, 0);
	}

	void VulkanRenderer::DrawIndexed(Ref<CommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer)
	{
		APP_PROFILE_SCOPE("VulkanRenderer::DrawIndexed");
		Renderer::GetRenderData().DrawCalls++;

		auto cmdBuf = RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer);
		vkCmdDrawIndexed(cmdBuf->GetVulkanCommandBuffer(m_SwapChain->GetCurrentFrame()), indexBuffer->GetCount(), 1, 0, 0, 0);
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		m_SwapChain->OnResize(width, height, Application::Get().GetWindow().IsVSync());
	}

}