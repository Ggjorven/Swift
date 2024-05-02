#include "DepthPrepass.hpp"

#include "ForwardPlus/Scene.hpp"

#include <Swift/Renderer/Renderer.hpp>

DepthPrepass::DepthPrepass(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
	m_DescriptorSets = DescriptorSets::Create({
		// Set 0 (We preallocate 10 sets)
		{ 10, { 0, {
			{ DescriptorType::DynamicUniformBuffer, 0, "u_Model", ShaderStage::Vertex }
		}}},

		// Set 1
		{ 1, { 1, {
			{ DescriptorType::UniformBuffer, 0, "u_Camera", ShaderStage::Vertex }
		}}}
	});

	CommandBufferSpecification cmdBufSpecs = {};
	cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

	auto cmdBuf = CommandBuffer::Create(cmdBufSpecs);

	RenderPassSpecification renderPassSpecs = {};
	renderPassSpecs.DepthLoadOp = LoadOperation::Clear;
	renderPassSpecs.DepthAttachment = Renderer::GetDepthImage();
	renderPassSpecs.PreviousDepthImageLayout = ImageLayout::Depth;
	renderPassSpecs.FinalDepthImageLayout = ImageLayout::Depth;

	m_DepthPass = RenderPass::Create(renderPassSpecs, cmdBuf);

	ShaderSpecification shaderSpecs = {};
	shaderSpecs.Vertex = cacher->GetLatest(compiler, "assets/shaders/caches/Depth.vert.cache", "assets/shaders/Depth.vert.glsl", ShaderStage::Vertex);
	shaderSpecs.Fragment = cacher->GetLatest(compiler, "assets/shaders/caches/Depth.frag.cache", "assets/shaders/Depth.frag.glsl", ShaderStage::Fragment);

	auto shader = Shader::Create(shaderSpecs);

	PipelineSpecification pipelineSpecs = {};
	pipelineSpecs.Bufferlayout = MeshVertex::GetLayout();
	pipelineSpecs.Polygonmode = PolygonMode::Fill;
	pipelineSpecs.Cullingmode = CullingMode::None;
	pipelineSpecs.LineWidth = 1.0f;
	pipelineSpecs.Blending = false;

	m_DepthPipeline = Pipeline::Create(pipelineSpecs, m_DescriptorSets, shader, m_DepthPass);
}

bool DepthPrepass::OnResize(WindowResizeEvent& e)
{
	m_DepthPass->Resize(e.GetWidth(), e.GetHeight());

	return false;
}

Ref<DepthPrepass> DepthPrepass::Create(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
	return RefHelper::Create<DepthPrepass>(compiler, cacher);
}
