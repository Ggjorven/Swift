#include "swpch.h"
#include "VulkanCommandBuffer.hpp"

#include "Swift/Core/Logging.hpp"
#include "Swift/Utils/Profiler.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanTaskManager.hpp"

namespace Swift
{

	VulkanCommandBuffer::VulkanCommandBuffer(CommandBufferSpecification specs)
		: m_Specification(specs)
	{
		// Check if specs are set properly
		if (!(m_Specification.Usage & CommandBufferUsage::Sequence) && !(m_Specification.Usage & CommandBufferUsage::Parallel))
		{
			APP_ASSERT(false, "No proper flags set.");
			return;
		}

		auto renderer = (VulkanRenderer*)Renderer::GetInstance();
		auto device = renderer->GetLogicalDevice()->GetVulkanDevice();
		constexpr const uint32_t framesInFlight = (uint32_t)RendererSpecification::BufferCount;
		m_CommandBuffers.resize(framesInFlight);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = renderer->GetSwapChain()->GetCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to allocate command buffers!");

		m_RenderFinishedSemaphores.resize(framesInFlight);
		m_InFlightFences.resize(framesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < framesInFlight; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
			{
				APP_LOG_ERROR("Failed to create synchronization objects for a frame!");
			}
		}
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		auto commandBuffers = m_CommandBuffers;
		auto renderFinishedSemaphores = m_RenderFinishedSemaphores;
		auto inFlightFences = m_InFlightFences;

		Renderer::SubmitFree([commandBuffers, renderFinishedSemaphores, inFlightFences]()
		{
			auto renderer = (VulkanRenderer*)Renderer::GetInstance();
			auto device = renderer->GetLogicalDevice()->GetVulkanDevice();

			vkDeviceWaitIdle(device);

			constexpr const uint32_t framesInFlight = (uint32_t)RendererSpecification::BufferCount;
			vkFreeCommandBuffers(device, renderer->GetSwapChain()->GetCommandPool(), framesInFlight, commandBuffers.data());

			for (size_t i = 0; i < (size_t)framesInFlight; i++)
			{
				vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
				vkDestroyFence(device, inFlightFences[i], nullptr);
			}
		});
	}

	void VulkanCommandBuffer::Begin()
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();
		auto device = renderer->GetLogicalDevice()->GetVulkanDevice();
		uint32_t currentFrame = Renderer::GetCurrentFrame();
		VkCommandBuffer commandBuffer = m_CommandBuffers[currentFrame];

		vkResetFences(device, 1, &m_InFlightFences[currentFrame]);
		vkResetCommandBuffer(commandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		{
			APP_PROFILE_SCOPE("VulkanCommandBuffer::Begin::Begin");
			if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
				APP_LOG_ERROR("Failed to begin recording command buffer!");
		}
	}

	void VulkanCommandBuffer::End()
	{
		APP_PROFILE_SCOPE("VulkanCommandBuffer::End::End");

		if (vkEndCommandBuffer(m_CommandBuffers[Renderer::GetCurrentFrame()]) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to record command buffer!");
	}

	void VulkanCommandBuffer::Submit(Queue queue, const std::vector<Ref<CommandBuffer>>& waitOn)
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();
		VkDevice device = renderer->GetLogicalDevice()->GetVulkanDevice();

		uint32_t currentFrame = Renderer::GetCurrentFrame();
		VkCommandBuffer commandBuffer = m_CommandBuffers[currentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		std::vector<VkSemaphore> semaphores = { };

		for (auto& cmd : waitOn)
		{
			auto vkCmd = RefHelper::RefAs<VulkanCommandBuffer>(cmd);
			auto semaphore = vkCmd->GetRenderFinishedSemaphore(currentFrame);

			semaphores.push_back(semaphore);
			if (VulkanTaskManager::HasSemaphore(semaphore))
				VulkanTaskManager::RemoveSemaphore(semaphore);
		}

		if (m_Specification.Usage & CommandBufferUsage::Sequence)
		{
			// Check if it's not nullptr
			if (VulkanTaskManager::GetFirstSempahore())
				semaphores.push_back(VulkanTaskManager::GetFirstSempahore());
		}

		std::vector<VkPipelineStageFlags> waitStages(semaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		submitInfo.waitSemaphoreCount = (uint32_t)semaphores.size();
		submitInfo.pWaitSemaphores = semaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[currentFrame];

		{
			APP_PROFILE_SCOPE("VulkanCommandBuffer::Submit::Queue");

			VkResult result = VK_SUCCESS;
			
			switch (queue)
			{
			case Queue::Graphics:
				vkQueueSubmit(renderer->GetLogicalDevice()->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[currentFrame]);
				break;
			case Queue::Compute:
				vkQueueSubmit(renderer->GetLogicalDevice()->GetComputeQueue(), 1, &submitInfo, m_InFlightFences[currentFrame]);
				break;

			default:
				APP_LOG_ERROR("Invalid queue selected.");
				break;
			}
			
			if (result != VK_SUCCESS)
				APP_LOG_ERROR("Failed to submit draw command buffer! Error: {0}", VkResultToString(result));
		}

		if (m_Specification.Usage & CommandBufferUsage::Sequence)
			VulkanTaskManager::AddSemaphore(m_RenderFinishedSemaphores[currentFrame]);
		if (m_Specification.Usage & CommandBufferUsage::Parallel)
			VulkanTaskManager::AddFrameSemaphore(m_RenderFinishedSemaphores[currentFrame]);

		VulkanTaskManager::AddFence(m_InFlightFences[currentFrame]);
	}

	void VulkanCommandBuffer::WaitOnFinish()
	{
		auto renderer = (VulkanRenderer*)Renderer::GetInstance();
		uint32_t currentFrame = Renderer::GetCurrentFrame();

		if (VulkanTaskManager::HasFence(m_InFlightFences[currentFrame]))
			VulkanTaskManager::RemoveFence(m_InFlightFences[currentFrame]);

		vkWaitForFences(renderer->GetLogicalDevice()->GetVulkanDevice(), 1, &m_InFlightFences[currentFrame], VK_TRUE, MAX_UINT64);
		vkResetFences(renderer->GetLogicalDevice()->GetVulkanDevice(), 1, &m_InFlightFences[currentFrame]);
	}

}