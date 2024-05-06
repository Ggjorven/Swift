#include "swpch.h"
#include "Renderer.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/RenderInstance.hpp"

#include "Swift/Renderer/Buffers.hpp"
#include "Swift/Renderer/CommandBuffer.hpp"

namespace Swift
{

	static RenderInstance* s_RenderInstance = nullptr;
	RendererSpecification Renderer::s_Specification = {};
	RenderData Renderer::s_Data = {};

	void Renderer::Init()
	{
		s_RenderInstance = RenderInstance::Create();
		s_RenderInstance->Init();
	}

	bool Renderer::Initialized()
	{
		return s_RenderInstance != nullptr;
	}

	void Renderer::Destroy()
	{
		delete s_RenderInstance;
	}

	void Renderer::BeginFrame()
	{
		s_RenderInstance->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RenderInstance->EndFrame();
	}

	void Renderer::Submit(RenderFunction function)
	{
		s_RenderInstance->Submit(function);
	}

	void Renderer::SubmitFree(FreeFunction function)
	{
		s_RenderInstance->SubmitFree(function);
	}

	void Renderer::Wait()
	{
		s_RenderInstance->Wait();
	}

	void Renderer::Draw(Ref<CommandBuffer> commandBuffer, uint32_t verticeCount)
	{
		s_RenderInstance->Draw(commandBuffer, verticeCount);
	}

	void Renderer::DrawIndexed(Ref<CommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer)
	{
		s_RenderInstance->DrawIndexed(commandBuffer, indexBuffer);
	}

	void Renderer::OnResize(uint32_t width, uint32_t height)
	{
		s_RenderInstance->OnResize(width, height);
	}

	Utils::Queue<RenderFunction>& Renderer::GetRenderQueue()
	{
		return s_RenderInstance->GetRenderQueue();
	}

	Utils::Queue<FreeFunction>& Renderer::GetFreeQueue()
	{
		return s_RenderInstance->GetFreeQueue();
	}

	uint32_t Renderer::GetCurrentFrame()
	{
		return s_RenderInstance->GetCurrentFrame();;
	}

	std::vector<Ref<Image2D>>& Renderer::GetSwapChainImages()
	{
		return s_RenderInstance->GetSwapChainImages();
	}

	Ref<Image2D> Renderer::GetDepthImage()
	{
		return s_RenderInstance->GetDepthImage();
	}

	RenderInstance* Renderer::GetInstance()
	{
		return s_RenderInstance;
	}

}