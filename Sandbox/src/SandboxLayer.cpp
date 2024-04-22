#include "SandboxLayer.hpp"

#include <Swift/Core/Application.hpp>
#include <Swift/Utils/Mesh.hpp>

#include <Swift/Renderer/Shader.hpp>

struct Vertex
{
public:
	glm::vec2 Position = { };
	glm::vec2 TexCoord = { };

public:
	inline static BufferLayout GetLayout()
	{
		static BufferLayout layout = {
			BufferElement(DataType::Float2, 0, "a_Position"),
			BufferElement(DataType::Float2, 1, "a_TexCoord")
		};

		return layout;
	}
};

static std::vector<Vertex> vertices = {
	{ { -1.0f, -1.0f }, { 0.0f, 0.0f } },
	{ {  1.0f, -1.0f }, { 1.0f, 0.0f } },
	{ {  1.0f,  1.0f }, { 1.0f, 1.0f } },
	{ { -1.0f,  1.0f }, { 0.0f, 1.0f } }
};

static std::vector<uint32_t> indices = {
	0, 1, 2,    2, 3, 0
};

void SandboxLayer::OnAttach()
{
	Ref<ShaderCompiler> compiler = ShaderCompiler::Create();
	
	// Compute
	{
		auto descriptorSets = DescriptorSets::Create({
			// Set 0
			{ 1, { 0, {
				{ DescriptorType::StorageImage, 0, "u_Image", ShaderStage::Compute, }
			}}},
		});

		CommandBufferSpecification cmdBufSpecs = {};
		cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

		m_ComputeCommandBuffer = CommandBuffer::Create(cmdBufSpecs);

		ShaderSpecification shaderSpecs = {};
		shaderSpecs.Compute = compiler->Compile(ShaderSpecification::ReadGLSLFile("assets/shaders/Test.comp.glsl"), ShaderStage::Compute);

		m_ComputeShader = ComputeShader::Create(shaderSpecs);

		PipelineSpecification pipelineSpecs = {};
		pipelineSpecs.Polygonmode = PolygonMode::Fill;
		pipelineSpecs.Cullingmode = CullingMode::None;
		pipelineSpecs.LineWidth = 1.0f;
		pipelineSpecs.Blending = false;

		m_ComputePipeline = Pipeline::Create(pipelineSpecs, descriptorSets, m_ComputeShader);
	}

	ImageSpecification imageSpecs = {};
	imageSpecs.Usage = ImageUsage::Size;
	imageSpecs.Flags = ImageUsageFlags::Storage;
	imageSpecs.Layout = ImageLayout::General;
	imageSpecs.Format = ImageFormat::RGBA;
	imageSpecs.Width = 1280;
	imageSpecs.Height = 720;

	m_Image = Image2D::Create(imageSpecs);

	// 2D
	{
		m_2DVertexBuffer = VertexBuffer::Create((void*)vertices.data(), sizeof(Vertex) * vertices.size());
		m_2DIndexBuffer = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());

		auto descriptorSets = DescriptorSets::Create({
			// Set 0
			{ 1, { 0, {
				{ DescriptorType::Image, 0, "u_Image", ShaderStage::Fragment, }
			}}},
		});

		CommandBufferSpecification cmdBufSpecs = {};
		cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

		auto cmdBuffer = CommandBuffer::Create(cmdBufSpecs);

		RenderPassSpecification renderPassSpecs = {};
		renderPassSpecs.Attachments = RenderPassAttachments::Colour | RenderPassAttachments::Depth;
		renderPassSpecs.ColourLoadOp = ColourLoadOperation::Clear;
		renderPassSpecs.ColourClearColour = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassSpecs.PreviousImageLayout = ImageLayout::Undefined;
		renderPassSpecs.FinalImageLayout = ImageLayout::Presentation;

		m_2DRenderPass = RenderPass::Create(renderPassSpecs, cmdBuffer);

		ShaderSpecification shaderSpecs = {};
		shaderSpecs.Vertex =	compiler->Compile(ShaderSpecification::ReadGLSLFile("assets/shaders/2D.vert.glsl"), ShaderStage::Vertex);
		shaderSpecs.Fragment =	compiler->Compile(ShaderSpecification::ReadGLSLFile("assets/shaders/2D.frag.glsl"), ShaderStage::Fragment);

		auto shader = Shader::Create(shaderSpecs);

		PipelineSpecification pipelineSpecs = {};
		pipelineSpecs.Bufferlayout = Vertex::GetLayout();
		pipelineSpecs.Polygonmode = PolygonMode::Fill;
		pipelineSpecs.Cullingmode = CullingMode::Front;
		pipelineSpecs.LineWidth = 1.0f;
		pipelineSpecs.Blending = false;

		m_2DPipeline = Pipeline::Create(pipelineSpecs, descriptorSets, shader, m_2DRenderPass);
	}
}

void SandboxLayer::OnDetach()
{
}

void SandboxLayer::OnUpdate(float deltaTime)
{
	static float timer = 0.0f;
	static uint32_t FPS = 0u;
	timer += deltaTime;
	FPS += 1u;

	if (timer >= 1.0f)
	{
		Application::Get().GetWindow().SetTitle(fmt::format("SandboxApp | FPS: {0} | Frametime: {1:.3f}ms", FPS, deltaTime * 1000.0f));
		timer = 0.0f;
		FPS = 0u;
	}
}

void SandboxLayer::OnRender()
{
	// Compute pass
	Renderer::Submit([this]()
	{
		m_ComputeCommandBuffer->Begin();

		m_ComputePipeline->Use(m_ComputeCommandBuffer, PipelineBindPoint::Compute);

		//m_Image->Transition(ImageLayout::Undefined, ImageLayout::General);
		m_Image->Upload(m_ComputePipeline->GetDescriptorSets()->GetSets(0)[0], m_ComputePipeline->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Image"));
		m_ComputePipeline->GetDescriptorSets()->GetSets(0)[0]->Bind(m_ComputePipeline, m_ComputeCommandBuffer, PipelineBindPoint::Compute);

		m_ComputeShader->Dispatch(m_ComputeCommandBuffer, m_Image->GetSpecification().Width / 16, m_Image->GetSpecification().Height / 16, 1);

		m_ComputeCommandBuffer->End();
		m_ComputeCommandBuffer->Submit(Queue::Compute);
	});

	// Render fullscreen quad
	/*
	Renderer::Submit([this]()
	{
		m_2DRenderPass->Begin();

		m_2DPipeline->Use(m_2DRenderPass->GetCommandBuffer(), PipelineBindPoint::Graphics);

		//m_Image->Transition(ImageLayout::Undefined, ImageLayout::ShaderRead);
		m_Image->Upload(m_2DPipeline->GetDescriptorSets()->GetSets(0)[0], m_2DPipeline->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Image"));
		m_2DPipeline->GetDescriptorSets()->GetSets(0)[0]->Bind(m_2DPipeline, m_2DRenderPass->GetCommandBuffer(), PipelineBindPoint::Graphics);

		m_2DVertexBuffer->Bind(m_2DRenderPass->GetCommandBuffer());
		m_2DIndexBuffer->Bind(m_2DRenderPass->GetCommandBuffer());
		Renderer::DrawIndexed(m_2DRenderPass->GetCommandBuffer(), m_2DIndexBuffer);

		m_2DRenderPass->End();
		m_2DRenderPass->Submit();
	});
	*/
}

void SandboxLayer::OnEvent(Event& e)
{
	EventHandler handler(e);

	handler.Handle<WindowResizeEvent>(APP_BIND_EVENT_FN(SandboxLayer::OnResize));
}

bool SandboxLayer::OnResize(WindowResizeEvent& e)
{
	m_2DRenderPass->Resize(e.GetWidth(), e.GetHeight());

	return false;
}
