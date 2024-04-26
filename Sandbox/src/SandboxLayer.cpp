#include "SandboxLayer.hpp"

#include <Swift/Core/Application.hpp>
#include <Swift/Core/Input/Input.hpp>
#include <Swift/Utils/Mesh.hpp>

#include <Swift/Renderer/Shader.hpp>

struct SceneSettings
{
public:
	glm::vec2 MousePos = {};
	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t Frame = 0;
	uint32_t FPS = 0;
	float DeltaTime = 0.0f;
	float Timer = 0.0f;
};

void SandboxLayer::OnAttach()
{
	Ref<ShaderCompiler> compiler = ShaderCompiler::Create();
	Ref<ShaderCacher> cacher = ShaderCacher::Create();
	
	// Compute
	{
		auto descriptorSets = DescriptorSets::Create({
			// Set 0
			{ 1, { 0, {
				{ DescriptorType::StorageImage, 0, "u_Image", ShaderStage::Compute, },
				{ DescriptorType::UniformBuffer, 1, "u_Timer", ShaderStage::Compute, }
			}}},
		});

		m_ComputeBuffer = UniformBuffer::Create(sizeof(SceneSettings));

		CommandBufferSpecification cmdBufSpecs = {};
		cmdBufSpecs.Usage = CommandBufferUsage::Parallel;

		m_ComputeCommandBuffer = CommandBuffer::Create(cmdBufSpecs);

		ShaderSpecification shaderSpecs = {};
		shaderSpecs.Compute = cacher->GetLatest(compiler, "assets/shaders/caches/Test.comp.cache", "assets/shaders/Test.comp.glsl", ShaderStage::Compute);

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
	imageSpecs.Flags = ImageUsageFlags::Storage | ImageUsageFlags::Sampled;
	imageSpecs.Layout = ImageLayout::ShaderRead;
	imageSpecs.Format = ImageFormat::RGBA;
	imageSpecs.Width = 1280;
	imageSpecs.Height = 720;

	m_Image = Image2D::Create(imageSpecs);

	// 2D
	{
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
		shaderSpecs.Vertex =	cacher->GetLatest(compiler, "assets/shaders/caches/Fullscreen.vert.cache", "assets/shaders/Fullscreen.vert.glsl", ShaderStage::Vertex);
		shaderSpecs.Fragment =	cacher->GetLatest(compiler, "assets/shaders/caches/Fullscreen.frag.cache", "assets/shaders/Fullscreen.frag.glsl", ShaderStage::Fragment);

		auto shader = Shader::Create(shaderSpecs);

		PipelineSpecification pipelineSpecs = {};
		pipelineSpecs.Bufferlayout = {};
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
	static float overallTimer = 0.0f;
	static uint32_t frame = 0;
	static float timer = 0.0f;
	static uint32_t FPS = 0;
	static uint32_t tempFPS = 0;
	timer += deltaTime;
	tempFPS += 1u;

	if (timer >= 1.0f)
	{
		FPS = (uint32_t)((float)tempFPS / timer);
		Application::Get().GetWindow().SetTitle(fmt::format("SandboxApp | FPS: {0} | Frametime: {1:.3f}ms", FPS, timer / (float)FPS * 1000.0f));
		timer = 0.0f;
		tempFPS = 0u;
	}

	overallTimer += deltaTime;
	frame += 1;

	SceneSettings settings = {};
	settings.MousePos = Input::GetMousePosition();
	settings.Width = Application::Get().GetWindow().GetWidth();
	settings.Height = Application::Get().GetWindow().GetHeight();
	settings.Frame = frame;
	settings.FPS = FPS;
	settings.DeltaTime = deltaTime;
	settings.Timer = overallTimer;

	m_ComputeBuffer->SetData((void*)&settings, sizeof(SceneSettings));
}

void SandboxLayer::OnRender()
{
	// Compute pass
	Renderer::Submit([this]()
	{
		m_ComputeCommandBuffer->Begin();

		m_ComputePipeline->Use(m_ComputeCommandBuffer, PipelineBindPoint::Compute);

		m_Image->Transition(ImageLayout::ShaderRead, ImageLayout::General);
		m_Image->Upload(m_ComputePipeline->GetDescriptorSets()->GetSets(0)[0], m_ComputePipeline->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Image"));

		m_ComputeBuffer->Upload(m_ComputePipeline->GetDescriptorSets()->GetSets(0)[0], m_ComputePipeline->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Timer"));

		m_ComputePipeline->GetDescriptorSets()->GetSets(0)[0]->Bind(m_ComputePipeline, m_ComputeCommandBuffer, PipelineBindPoint::Compute);

		m_ComputeShader->Dispatch(m_ComputeCommandBuffer, m_Image->GetSpecification().Width / 16 + 1, m_Image->GetSpecification().Height / 16 + 1, 1);

		m_ComputeCommandBuffer->End();
		m_ComputeCommandBuffer->Submit(Queue::Compute);
	});

	// Render fullscreen
	Renderer::Submit([this]()
	{
		m_2DRenderPass->Begin();

		m_2DPipeline->Use(m_2DRenderPass->GetCommandBuffer(), PipelineBindPoint::Graphics);

		m_Image->Transition(ImageLayout::General, ImageLayout::ShaderRead);
		m_Image->Upload(m_2DPipeline->GetDescriptorSets()->GetSets(0)[0], m_2DPipeline->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Image"));

		m_2DPipeline->GetDescriptorSets()->GetSets(0)[0]->Bind(m_2DPipeline, m_2DRenderPass->GetCommandBuffer(), PipelineBindPoint::Graphics);

		Renderer::Draw(m_2DRenderPass->GetCommandBuffer(), 3);

		m_2DRenderPass->End();
		m_2DRenderPass->Submit({ m_ComputeCommandBuffer });
	});
}

void SandboxLayer::OnEvent(Event& e)
{
	EventHandler handler(e);

	handler.Handle<WindowResizeEvent>(APP_BIND_EVENT_FN(SandboxLayer::OnResize));
}

bool SandboxLayer::OnResize(WindowResizeEvent& e)
{
	m_Image->Resize(e.GetWidth(), e.GetHeight());
	m_2DRenderPass->Resize(e.GetWidth(), e.GetHeight());

	return false;
}
