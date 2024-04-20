#pragma once

#include <stdint.h>
#include <memory>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/RendererConfig.hpp"

namespace Swift
{

	class CommandBuffer;
	class RenderInstance;
	class IndexBuffer;

	class Renderer
	{
	public:
		static void Init();
		static bool Initialized();
		static void Destroy();

		static void BeginFrame();
		static void EndFrame();

		static void Submit(RenderFunction function);
		static void SubmitFree(FreeFunction function);

		static void Wait();

		//static void DrawIndexed(Ref<CommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer);

		static void OnResize(uint32_t width, uint32_t height);

		static Utils::Queue<RenderFunction>& GetRenderQueue();
		static Utils::Queue<FreeFunction>& GetFreeQueue();
		inline static RenderData& GetRenderData() { return s_Data; }

		static RenderInstance* GetInstance();
		
	private:
		static RendererSpecification s_Specification;
		static RenderData s_Data;
	};

}