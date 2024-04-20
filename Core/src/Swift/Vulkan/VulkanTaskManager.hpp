#pragma once

#include <vector>
#include <queue>
#include <mutex>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include <vulkan/vulkan.h>

namespace Swift
{

	// A threadsafe class for keeping track of things to wait on
	class VulkanTaskManager
	{
	public:
		static void AddSemaphore(VkSemaphore semaphore);
		static void AddFence(VkFence fence);

		// Every function below either clears the item from the list or the entire list
		static VkSemaphore GetFirstSempahore();
		static VkFence GetFirstFence();

		static std::vector<VkSemaphore>& GetSemaphores();
		static std::vector<VkFence>& GetFences();

	private:
		static std::mutex s_SemaphoreMutex;
		static Dict<uint32_t, std::queue<VkSemaphore>> s_Semaphores;

		static std::mutex s_FenceMutex;
		static Dict<uint32_t, std::queue<VkFence>> s_Fences;
	};

}