#include "swpch.h"
#include "VulkanImGuiLayer.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "Swift/Core/Application.hpp"
#include "Swift/Core/Logging.hpp"
#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

namespace Swift
{

	static VkDescriptorPool s_ImGuiPool = VK_NULL_HANDLE;

	void VulkanImGuiLayer::OnAttach()
	{
		auto context = (VulkanRenderer*)Renderer::GetInstance();
		{
			std::vector<VkDescriptorPoolSize> poolSizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = 1000 * (uint32_t)RendererSpecification::BufferCount;

			if (vkCreateDescriptorPool(context->GetLogicalDevice()->GetVulkanDevice(), &poolInfo, nullptr, &s_ImGuiPool) != VK_SUCCESS)
				APP_LOG_ERROR("Failed to create descriptor pool!");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		io.IniFilename = NULL;

		ImGui::StyleColorsDark();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		ImGui_ImplGlfw_InitForVulkan(window, true);

		ImGui_ImplVulkan_InitInfo  initInfo = {};
		initInfo.Instance = context->GetVulkanInstance();
		initInfo.PhysicalDevice = context->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		initInfo.Device = context->GetLogicalDevice()->GetVulkanDevice();
		initInfo.QueueFamily = QueueFamilyIndices::Find(context->GetPhysicalDevice()->GetVulkanPhysicalDevice()).GraphicsFamily.value();
		initInfo.Queue = context->GetLogicalDevice()->GetGraphicsQueue();
		//initInfo.PipelineCache = vkPipelineCache;
		initInfo.DescriptorPool = s_ImGuiPool;
		initInfo.Allocator = nullptr; // Optional, use nullptr to use the default allocator
		initInfo.MinImageCount = (uint32_t)Renderer::GetSwapChainImages().size();
		initInfo.ImageCount = (uint32_t)Renderer::GetSwapChainImages().size();
		initInfo.CheckVkResultFn = nullptr; // Optional, a callback function for Vulkan errors
		//init_info.MSAASamples = vkMSAASamples; // The number of samples per pixel in case of MSAA
		//init_info.Subpass = 0; // The index of the subpass where ImGui will be drawn

		// Create renderpass
		RenderPassSpecification specs = {};
		specs.ColourAttachment = Renderer::GetSwapChainImages();
		specs.ColourLoadOp = LoadOperation::Load; 	// To not overwrite previous colour attachments
		specs.PreviousColourImageLayout = ImageLayout::Presentation; // Because before this pass there is pretty much always a renderpass with Presentation

		m_Renderpass = RefHelper::Create<VulkanRenderPass>(specs, CommandBuffer::Create({}));
		
		ImGui_ImplVulkan_Init(&initInfo, m_Renderpass->GetVulkanRenderPass());

		// Create fonts
		io.Fonts->AddFontDefault();
		{
			auto command = VulkanCommand(true);
			ImGui_ImplVulkan_CreateFontsTexture(command.GetVulkanCommandBuffer());
			command.EndAndSubmit();

			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	void VulkanImGuiLayer::OnDetach()
	{
		auto pool = s_ImGuiPool;

		Renderer::SubmitFree([pool]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

			vkDestroyDescriptorPool(device, pool, nullptr);

			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		});
	}

	void VulkanImGuiLayer::Begin()
	{
		m_Renderpass->Begin();

		{
			APP_PROFILE_SCOPE("VulkanImGuiLayer::Begin::NewFrame");
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	void VulkanImGuiLayer::End()
	{
		APP_PROFILE_SCOPE("VulkanImGuiLayer::Begin::EndFrame");

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)Application::Get().GetWindow().GetWidth(), (float)Application::Get().GetWindow().GetHeight());

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), RefHelper::RefAs<VulkanCommandBuffer>(m_Renderpass->GetCommandBuffer())->GetVulkanCommandBuffer(Renderer::GetCurrentFrame()));

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		m_Renderpass->End();
		m_Renderpass->Submit({});
	}

	void VulkanImGuiLayer::Resize(uint32_t width, uint32_t height)
	{
		m_Renderpass->Resize(width, height);
	}

	VkDescriptorPool VulkanImGuiLayer::GetVulkanDescriptorPool()
	{
		return s_ImGuiPool;
	}

}