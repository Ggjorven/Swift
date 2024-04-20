#pragma once

#include <functional>

#include "Swift/Core/Core.hpp"

namespace Swift
{

	typedef std::function<void()> RenderFunction;
	typedef std::function<void()> FreeFunction;

	struct RendererSpecification
	{
	public:
		enum class RenderingAPI
		{
			None = 0, Vulkan
		};
		enum class BufferMode
		{
			None = 0, Double = 2, Triple = 3
		};
	public:
		inline static constexpr const RenderingAPI API = RenderingAPI::Vulkan;
		inline static constexpr const BufferMode BufferCount = BufferMode::Triple;
	};

	struct RenderData
	{
	public:
		uint32_t DrawCalls = 0;

	public:
		inline void Reset()
		{
			DrawCalls = 0;
		}
	};

}