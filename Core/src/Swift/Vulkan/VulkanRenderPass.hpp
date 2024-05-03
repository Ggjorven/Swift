#pragma once

#include <vector>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/RenderPass.hpp"
#include "Swift/Renderer/CommandBuffer.hpp"

#include "Swift/Vulkan/VulkanCommandBuffer.hpp"

#include <vulkan/vulkan.h>

namespace Swift
{

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(RenderPassSpecification specs, Ref<CommandBuffer> commandBuffer);
		virtual ~VulkanRenderPass();

		void Begin() override;
		void End() override;
		void Submit(const std::vector<Ref<CommandBuffer>>& waitOn) override;

		void Resize(uint32_t width, uint32_t height) override;

		inline RenderPassSpecification& GetSpecification() override { return m_Specification; }
		inline Ref<CommandBuffer> GetCommandBuffer() override { return m_CommandBuffer; }

		inline VkRenderPass& GetVulkanRenderPass() { return m_RenderPass; }
		inline std::vector<VkFramebuffer> GetFrameBuffers() { return m_Framebuffers; };

	private:
		void Create();
		void Destroy();

	private:
		RenderPassSpecification m_Specification = {};

		Ref<VulkanCommandBuffer> m_CommandBuffer = VK_NULL_HANDLE;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		std::vector<VkFramebuffer> m_Framebuffers = { };
	};

}