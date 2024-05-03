#pragma once

#include "Swift/Utils/BaseImGuiLayer.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Vulkan/VulkanRenderPass.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Swift
{

	class VulkanImGuiLayer : public BaseImGuiLayer
	{
	public:
		VulkanImGuiLayer() = default;
		virtual ~VulkanImGuiLayer() = default;

		void OnAttach() override;
		void OnDetach() override;

		void Begin() override;
		void End() override;

		void Resize(uint32_t width, uint32_t height) override;

		VkDescriptorPool GetVulkanDescriptorPool();

	private:
		Ref<VulkanRenderPass> m_Renderpass = VK_NULL_HANDLE;
	};

}