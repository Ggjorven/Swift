#include "Scene.hpp"

#include <Swift/Utils/Utils.hpp>

Scene::Scene()
{
	Ref<ShaderCompiler> compiler = ShaderCompiler::Create();
	Ref<ShaderCacher> cacher = ShaderCacher::Create();

	// We preallocate 10 model matrices
	m_ModelBuffer = DynamicUniformBuffer::Create(10, sizeof(Model));
	m_CameraBuffer = UniformBuffer::Create(sizeof(Camera));

	m_DepthPrepass = DepthPrepass::Create(compiler, cacher);
	m_LightCulling = LightCulling::Create(compiler, cacher);
	m_FinalShading = FinalShading::Create(compiler, cacher);

	// We transition the depth image to DepthRead since the depth prepass goes from DepthRead to Depth
	//Renderer::GetDepthImage()->Transition(ImageLayout::Depth, ImageLayout::DepthRead);

	// Playground/Sandbox
	auto mesh = Mesh::Create("assets/objects/viking_room.obj");
	auto material = Image2D::Create(ImageSpecification("assets/objects/viking_room.png"));
	Ref<Entity> entity = Entity::Create(mesh, material);
	//entity->GetPosition() = { 0.0f, -1.0f, 0.0f };
	entity->GetRotation() = { -90.0f, 0.0, 180.0f };
	m_Entities.push_back(entity);

	PointLight light = {};
	//light.Position = { 0.0f, -1.0f, 0.0f };
	m_PointLights.push_back(light);
	m_PointLights.push_back(light);
}

void Scene::OnUpdate(float deltaTime)
{
	{
		// TODO: Add some camera updating logic

		glm::vec3 cameraPosition = glm::vec3(0.0f, 3.0f, 3.0f);
		glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);

		Camera camera = {};
		camera.View = glm::lookAt(cameraPosition, cameraTarget, { 0.0f, 1.0f, 0.0f });
		camera.Projection = glm::perspective(glm::radians(45.0f), (float)Application::Get().GetWindow().GetWidth() / (float)Application::Get().GetWindow().GetHeight(), 0.1f, 1000.0f);
		if (RendererSpecification::API == RendererSpecification::RenderingAPI::Vulkan)
			camera.Projection[1][1] *= -1;

		float depthLinearizeMul = (-camera.Projection[3][2]);
		float depthLinearizeAdd = (camera.Projection[2][2]);
		// correct the handedness issue.
		if (depthLinearizeMul * depthLinearizeAdd < 0)
			depthLinearizeAdd = -depthLinearizeAdd;
		camera.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };

		// Upload data
		m_CameraBuffer->SetData(&camera, sizeof(Camera));

		m_CameraBuffer->Upload(m_DepthPrepass->GetDescriptorSets()->GetSets(1)[0], m_DepthPrepass->GetDescriptorSets()->GetLayout(1).GetDescriptorByName("u_Camera"));
		m_CameraBuffer->Upload(m_LightCulling->GetDescriptorSets()->GetSets(1)[0], m_LightCulling->GetDescriptorSets()->GetLayout(1).GetDescriptorByName("u_Camera"));
		m_CameraBuffer->Upload(m_FinalShading->GetDescriptorSets()->GetSets(1)[0], m_FinalShading->GetDescriptorSets()->GetLayout(1).GetDescriptorByName("u_Camera"));
	}

	{
		// TODO: Check if we need to resize 
		
		// Model matrix updating
		std::vector<Model> matrices(m_Entities.size());
		for (size_t i = 0; i < m_Entities.size(); i++)
		{
			matrices[i] = { m_Entities[i]->CalculateMatrix() };
			m_ModelBuffer->SetDataIndexed((uint32_t)i, &matrices[i], sizeof(Model));
		}
		m_ModelBuffer->UploadIndexedData();
	}

	{
		// Point light updating
		auto& set0 = m_LightCulling->GetDescriptorSets()->GetSets(0)[0];
		auto& set1 = m_LightCulling->GetDescriptorSets()->GetSets(1)[0];

		uint32_t size = (uint32_t)m_PointLights.size();
		m_LightCulling->GetLightsBuffer()->SetData((void*)&size, sizeof(uint32_t));
		m_LightCulling->GetLightsBuffer()->SetData((void*)m_PointLights.data(), sizeof(PointLight) * m_PointLights.size(), sizeof(uint32_t) * 4); // * 4 since uint32_t is 4 bytes but we also have 12 bytes of padding
		m_LightCulling->GetLightsBuffer()->Upload(set0, m_LightCulling->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Lights"));

		m_LightCulling->GetLightVisibilityBuffer()->Upload(set0, m_LightCulling->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Visibility"));
	}

	{
		SceneUniform uniform = { { Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight() } };
		m_LightCulling->GetSceneBuffer()->SetData((void*)&uniform, sizeof(SceneUniform));
		m_LightCulling->GetSceneBuffer()->Upload(m_LightCulling->GetDescriptorSets()->GetSets(1)[0], m_LightCulling->GetDescriptorSets()->GetLayout(1).GetDescriptorByName("u_Scene"));
		m_LightCulling->GetSceneBuffer()->Upload(m_FinalShading->GetDescriptorSets()->GetSets(1)[0], m_LightCulling->GetDescriptorSets()->GetLayout(1).GetDescriptorByName("u_Scene"));
	}
}

void Scene::OnRender()
{
	// Depth pre pass
	Renderer::Submit([this]()
	{
		auto& modelSets = m_DepthPrepass->GetDescriptorSets()->GetSets(0);
		auto& cameraSet = m_DepthPrepass->GetDescriptorSets()->GetSets(1)[0];

		//Renderer::GetDepthImage()->Transition(ImageLayout::DepthRead, ImageLayout::Depth);

		m_DepthPrepass->GetRenderPass()->Begin();

		m_DepthPrepass->GetPipeline()->Use(m_DepthPrepass->GetRenderPass()->GetCommandBuffer());
		cameraSet->Bind(m_DepthPrepass->GetPipeline(), m_DepthPrepass->GetRenderPass()->GetCommandBuffer());

		for (size_t i = 0; i < m_Entities.size(); i++)
		{
			auto& entity = m_Entities[i];

			m_ModelBuffer->Upload(modelSets[i], m_DepthPrepass->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Model"), sizeof(Model) * i);
			modelSets[i]->Bind(m_DepthPrepass->GetPipeline(), m_DepthPrepass->GetRenderPass()->GetCommandBuffer(), PipelineBindPoint::Graphics, { 0 });
			
			entity->BindMesh(m_DepthPrepass->GetRenderPass()->GetCommandBuffer());

			Renderer::DrawIndexed(m_DepthPrepass->GetRenderPass()->GetCommandBuffer(), entity->GetMesh()->GetIndexBuffer());
		}

		m_DepthPrepass->GetRenderPass()->End();
		m_DepthPrepass->GetRenderPass()->Submit();
	});

	// Light culling
	Renderer::Submit([this]() 
	{
		const TileCount tiles = m_LightCulling->GetTileCount();
		auto cmdBuffer = m_LightCulling->GetCommandBuffer();
		auto& set0 = m_LightCulling->GetDescriptorSets()->GetSets(0)[0];
		auto& set1 = m_LightCulling->GetDescriptorSets()->GetSets(1)[0];

		cmdBuffer->Begin();

		Renderer::GetDepthImage()->Transition(ImageLayout::Depth, ImageLayout::DepthRead);
		Renderer::GetDepthImage()->Upload(set0, m_LightCulling->GetPipeline()->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_DepthBuffer"));

		m_LightCulling->GetPipeline()->Use(cmdBuffer, PipelineBindPoint::Compute);

		set0->Bind(m_LightCulling->GetPipeline(), cmdBuffer, PipelineBindPoint::Compute);
		set1->Bind(m_LightCulling->GetPipeline(), cmdBuffer, PipelineBindPoint::Compute);

		m_LightCulling->GetComputeShader()->Dispatch(cmdBuffer, tiles.X, tiles.Y, 1);

		cmdBuffer->End();
		cmdBuffer->Submit(Queue::Compute);
	});

	// Final shading
	Renderer::Submit([this]()
	{
		auto& modelSets = m_FinalShading->GetDescriptorSets()->GetSets(0);
		auto& cameraSet = m_FinalShading->GetDescriptorSets()->GetSets(1)[0];

		m_FinalShading->GetRenderPass()->Begin();

		m_FinalShading->GetPipeline()->Use(m_FinalShading->GetRenderPass()->GetCommandBuffer());
		cameraSet->Bind(m_FinalShading->GetPipeline(), m_FinalShading->GetRenderPass()->GetCommandBuffer());

		for (size_t i = 0; i < m_Entities.size(); i++)
		{
			auto& entity = m_Entities[i];

			m_ModelBuffer->Upload(modelSets[i], m_FinalShading->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Model"), sizeof(Model) * i);
			entity->GetMaterial().Image->Upload(modelSets[i], m_FinalShading->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Albedo"));
			m_LightCulling->GetLightsBuffer()->Upload(modelSets[i], m_FinalShading->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Lights"));
			m_LightCulling->GetLightVisibilityBuffer()->Upload(modelSets[i], m_FinalShading->GetDescriptorSets()->GetLayout(0).GetDescriptorByName("u_Visibility"));
			
			modelSets[i]->Bind(m_FinalShading->GetPipeline(), m_FinalShading->GetRenderPass()->GetCommandBuffer(), PipelineBindPoint::Graphics, { 0 });
			
			entity->BindMesh(m_FinalShading->GetRenderPass()->GetCommandBuffer());

			Renderer::DrawIndexed(m_FinalShading->GetRenderPass()->GetCommandBuffer(), entity->GetMesh()->GetIndexBuffer());
		}

		m_FinalShading->GetRenderPass()->End();
		m_FinalShading->GetRenderPass()->Submit();
	});
}

void Scene::OnEvent(Event& e)
{
	EventHandler handler(e);

	handler.Handle<WindowResizeEvent>(APP_BIND_EVENT_FN(Scene::OnResize));
}

Ref<Scene> Scene::Create()
{
	return RefHelper::Create<Scene>();
}

bool Scene::OnResize(WindowResizeEvent& e)
{
	m_DepthPrepass->OnResize(e);
	m_LightCulling->OnResize(e);
	m_FinalShading->OnResize(e);

	return false;
}
