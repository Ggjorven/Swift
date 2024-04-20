#include "swpch.h"
#include "VulkanTaskManager.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanRenderer.hpp"

namespace Swift
{

	std::mutex									VulkanTaskManager::s_SemaphoreMutex;
	Dict<uint32_t, std::queue<VkSemaphore>>		VulkanTaskManager::s_Semaphores;

	std::mutex									VulkanTaskManager::s_FenceMutex;
	Dict<uint32_t, std::queue<VkFence>>			VulkanTaskManager::s_Fences;



	void VulkanTaskManager::AddSemaphore(VkSemaphore semaphore)
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);

		uint32_t frame = ((VulkanRenderer*)Renderer::GetInstance())->GetSwapChain()->GetCurrentFrame();
		s_Semaphores[frame].push(semaphore);
	}

	void VulkanTaskManager::AddFence(VkFence fence)
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		uint32_t frame = ((VulkanRenderer*)Renderer::GetInstance())->GetSwapChain()->GetCurrentFrame();
		s_Fences[frame].push(fence);
	}

	VkSemaphore VulkanTaskManager::GetFirstSempahore()
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);

		uint32_t frame = ((VulkanRenderer*)Renderer::GetInstance())->GetSwapChain()->GetCurrentFrame();
		auto semaphore = std::move(s_Semaphores[frame].front());
		s_Semaphores[frame].pop();
		return semaphore;
	}

	VkFence VulkanTaskManager::GetFirstFence()
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		uint32_t frame = ((VulkanRenderer*)Renderer::GetInstance())->GetSwapChain()->GetCurrentFrame();
		auto fence = std::move(s_Fences[frame].front());
		s_Fences[frame].pop();
		return fence;
	}

	std::vector<VkSemaphore>& VulkanTaskManager::GetSemaphores()
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);

		static std::vector<VkSemaphore> semaphores = { };
		semaphores.clear();
		
		uint32_t frame = ((VulkanRenderer*)Renderer::GetInstance())->GetSwapChain()->GetCurrentFrame();
		while (!s_Semaphores[frame].empty())
		{
			semaphores.push_back(std::move(s_Semaphores[frame].front()));
			s_Semaphores[frame].pop();
		}

		return semaphores;
	}

	std::vector<VkFence>& VulkanTaskManager::GetFences()
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		static std::vector<VkFence> fences = { };
		fences.clear();

		uint32_t frame = ((VulkanRenderer*)Renderer::GetInstance())->GetSwapChain()->GetCurrentFrame();
		while (!s_Fences[frame].empty())
		{
			fences.push_back(std::move(s_Fences[frame].front()));
			s_Fences[frame].pop();
		}

		return fences;
	}

}