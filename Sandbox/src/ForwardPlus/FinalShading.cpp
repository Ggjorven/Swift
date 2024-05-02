#include "FinalShading.hpp"

#include "ForwardPlus/Scene.hpp"

#include <Swift/Renderer/Renderer.hpp>

FinalShading::FinalShading(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
	m_DescriptorSets = DescriptorSets::Create({
		// Set 0 (We preallocate 10 sets)
		{ 10, { 0, {
			{ DescriptorType::DynamicUniformBuffer, 0, "u_Model", ShaderStage::Vertex },
			{ DescriptorType::Image, 1, "u_Albedo", ShaderStage::Fragment },
			{ DescriptorType::StorageBuffer, 2, "u_Lights", ShaderStage::Fragment },
			{ DescriptorType::StorageBuffer, 3, "u_Visibility", ShaderStage::Fragment }
		}}},

		// Set 1
		{ 1, { 1, {
			{ DescriptorType::UniformBuffer, 0, "u_Camera", ShaderStage::Vertex },
			{ DescriptorType::UniformBuffer, 1, "u_Scene", ShaderStage::Fragment }
		}}}
	});

	CommandBufferSpecification cmdBufSpecs = {};
	cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

	auto cmdBuf = CommandBuffer::Create(cmdBufSpecs);

	RenderPassSpecification renderPassSpecs = {};
	renderPassSpecs.ColourAttachment = Renderer::GetSwapChainImages();
	renderPassSpecs.ColourLoadOp = LoadOperation::Clear;
	renderPassSpecs.ColourClearColour = { 0.0f, 0.0f, 0.0f, 1.0f };
	renderPassSpecs.PreviousColourImageLayout = ImageLayout::Undefined;
	renderPassSpecs.FinalColourImageLayout = ImageLayout::Presentation;

	renderPassSpecs.DepthLoadOp = LoadOperation::Clear;
	renderPassSpecs.DepthAttachment = Renderer::GetDepthImage();
	renderPassSpecs.PreviousDepthImageLayout = ImageLayout::DepthRead;
	renderPassSpecs.FinalDepthImageLayout = ImageLayout::Depth;
	
	m_RenderPass = RenderPass::Create(renderPassSpecs, cmdBuf);

	ShaderSpecification shaderSpecs = {};
	shaderSpecs.Vertex = cacher->GetLatest(compiler, "assets/shaders/caches/Shading.vert.cache", "assets/shaders/Shading.vert.glsl", ShaderStage::Vertex);
	shaderSpecs.Fragment = cacher->GetLatest(compiler, "assets/shaders/caches/Shading.frag.cache", "assets/shaders/Shading.frag.glsl", ShaderStage::Fragment);

	auto shader = Shader::Create(shaderSpecs);

	PipelineSpecification pipelineSpecs = {};
	pipelineSpecs.Bufferlayout = MeshVertex::GetLayout();
	pipelineSpecs.Polygonmode = PolygonMode::Fill;
	pipelineSpecs.Cullingmode = CullingMode::Back;
	pipelineSpecs.LineWidth = 1.0f;
	pipelineSpecs.Blending = false;

	m_Pipeline = Pipeline::Create(pipelineSpecs, m_DescriptorSets, shader, m_RenderPass);
}

bool FinalShading::OnResize(WindowResizeEvent& e)
{
	m_RenderPass->Resize(e.GetWidth(), e.GetHeight());

	return false;
}

Ref<FinalShading> FinalShading::Create(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
	return RefHelper::Create<FinalShading>(compiler, cacher);
}
