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
	class Image2D;

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
		static void SubmitUI(UIFunction function);

		static void Wait();

		static void Draw(Ref<CommandBuffer> commandBuffer, uint32_t verticeCount = 3);
		static void DrawIndexed(Ref<CommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer);

		static void OnResize(uint32_t width, uint32_t height);

		static Utils::Queue<RenderFunction>& GetRenderQueue();
		static Utils::Queue<FreeFunction>& GetFreeQueue();

		static uint32_t GetCurrentFrame();
		static std::vector<Ref<Image2D>>& GetSwapChainImages();
		static Ref<Image2D> GetDepthImage();

		inline static RenderData& GetRenderData() { return s_Data; }

		static RenderInstance* GetInstance();
		
	private:
		static RendererSpecification s_Specification;
		static RenderData s_Data;
	};

}