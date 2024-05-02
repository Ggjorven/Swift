#include "swpch.h"
#include "VulkanTaskManager.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanRenderer.hpp"

namespace Swift
{

	std::mutex									VulkanTaskManager::s_SemaphoreMutex;
	Dict<uint32_t, LinkedList<VkSemaphore>>		VulkanTaskManager::s_Semaphores;
	std::mutex									VulkanTaskManager::s_FrameSemaphoreMutex;
	Dict<uint32_t, LinkedList<VkSemaphore>>		VulkanTaskManager::s_FrameSemaphores;

	std::mutex									VulkanTaskManager::s_FenceMutex;
	Dict<uint32_t, LinkedList<VkFence>>			VulkanTaskManager::s_Fences;



	void VulkanTaskManager::AddSemaphore(VkSemaphore semaphore)
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);

		s_Semaphores[Renderer::GetCurrentFrame()].Push(semaphore);
	}

	void VulkanTaskManager::AddFrameSemaphore(VkSemaphore semaphore)
	{
		std::scoped_lock<std::mutex> lock(s_FrameSemaphoreMutex);

		s_FrameSemaphores[Renderer::GetCurrentFrame()].Push(semaphore);
	}

	void VulkanTaskManager::AddFence(VkFence fence)
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		s_Fences[Renderer::GetCurrentFrame()].Push(fence);
	}

	VkSemaphore& VulkanTaskManager::GetFirstSempahore()
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);

		static VkSemaphore semaphore = VK_NULL_HANDLE;

		uint32_t frame = Renderer::GetCurrentFrame();

		if (!s_Semaphores[frame].Empty())
		{
			semaphore = std::move(s_Semaphores[frame].Front());
			s_Semaphores[frame].Pop();
		}

		return semaphore;
	}

	VkFence& VulkanTaskManager::GetFirstFence()
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		static VkFence fence = VK_NULL_HANDLE;

		uint32_t frame = Renderer::GetCurrentFrame();

		if (!s_Fences[frame].Empty())
		{
			fence = std::move(s_Fences[frame].Front());
			s_Fences[frame].Pop();
		}

		return fence;
	}

	std::vector<VkSemaphore>& VulkanTaskManager::GetSemaphores()
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);

		static std::vector<VkSemaphore> semaphores = { };
		semaphores.clear();

		uint32_t frame = Renderer::GetCurrentFrame();
		semaphores = s_Semaphores[frame].AsVector();
		semaphores.insert(semaphores.end(), s_FrameSemaphores[frame].AsVector().begin(), s_FrameSemaphores[frame].AsVector().end());

		s_Semaphores[frame].Clear();
		s_FrameSemaphores[frame].Clear();

		return semaphores;
	}

	std::vector<VkFence>& VulkanTaskManager::GetFences()
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		static std::vector<VkFence> fences = { };
		fences.clear();

		uint32_t frame = Renderer::GetCurrentFrame();
		fences = s_Fences[frame].AsVector();

		s_Fences[frame].Clear();

		return fences;
	}

	bool VulkanTaskManager::HasSemaphore(VkSemaphore semaphore)
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);
		std::scoped_lock<std::mutex> lock2(s_FrameSemaphoreMutex);

		uint32_t frame = Renderer::GetCurrentFrame();

		return s_Semaphores[frame].HasItem(semaphore) || s_FrameSemaphores[frame].HasItem(semaphore);
	}

	void VulkanTaskManager::RemoveSemaphore(VkSemaphore semaphore)
	{
		std::scoped_lock<std::mutex> lock(s_SemaphoreMutex);
		std::scoped_lock<std::mutex> lock2(s_FrameSemaphoreMutex);

		uint32_t frame = Renderer::GetCurrentFrame();

		if (s_Semaphores[frame].HasItem(semaphore))
			s_Semaphores[frame].Remove(semaphore);
		else if (s_FrameSemaphores[frame].HasItem(semaphore))
			s_FrameSemaphores[frame].Remove(semaphore);
		else
		{
			APP_ASSERT(false, "Semaphore by value {0} doesn't exist.", (void*)semaphore)
			return;
		}
	}

	bool VulkanTaskManager::HasFence(VkFence fence)
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		uint32_t frame = Renderer::GetCurrentFrame();

		return s_Fences[frame].HasItem(fence);
	}

	void VulkanTaskManager::RemoveFence(VkFence fence)
	{
		std::scoped_lock<std::mutex> lock(s_FenceMutex);

		uint32_t frame = Renderer::GetCurrentFrame();

		if (s_Fences[frame].HasItem(fence))
			s_Fences[frame].Remove(fence);
		else
		{
			APP_ASSERT(false, "Fence by value {0} doesn't exist.", (void*)fence)
			return;
		}
	}

}