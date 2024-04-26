#pragma once

#include <Swift/Core/Layer.hpp>

#include <Swift/Renderer/CommandBuffer.hpp>
#include <Swift/Renderer/RenderPass.hpp>
#include <Swift/Renderer/Image.hpp>

#include <Swift/Renderer/Buffers.hpp>
#include <Swift/Renderer/Pipeline.hpp>
#include <Swift/Renderer/Descriptors.hpp>

using namespace Swift;

class SandboxLayer : public Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(float deltaTime);
	void OnRender();
	void OnEvent(Event& e);

private:
	bool OnResize(WindowResizeEvent& e);

private:
	Ref<Pipeline> m_ComputePipeline = nullptr;
	Ref<ComputeShader> m_ComputeShader = nullptr;
	Ref<CommandBuffer> m_ComputeCommandBuffer = nullptr;
	Ref<UniformBuffer> m_ComputeBuffer = nullptr;
	
	Ref<Image2D> m_Image = nullptr;

	Ref<Pipeline> m_2DPipeline = nullptr;
	Ref<RenderPass> m_2DRenderPass = nullptr;
};