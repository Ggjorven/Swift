#pragma once

#include <Swift/Core/Layer.hpp>

#include <Swift/Renderer/CommandBuffer.hpp>
#include <Swift/Renderer/RenderPass.hpp>
#include <Swift/Renderer/Image.hpp>

#include <Swift/Renderer/Shader.hpp>
#include <Swift/Renderer/Buffers.hpp>
#include <Swift/Renderer/Pipeline.hpp>
#include <Swift/Renderer/Descriptors.hpp>

#include "ForwardPlus/Core.hpp"
#include "ForwardPlus/DepthPrepass.hpp"
#include "ForwardPlus/LightCulling.hpp"
#include "ForwardPlus/FinalShading.hpp"
#include "ForwardPlus/SceneItems.hpp"

using namespace Swift;

class Scene
{
public:
	Scene();
	virtual ~Scene() = default;

	void OnUpdate(float deltaTime);
	void OnRender();
	void OnEvent(Event& e);

	static Ref<Scene> Create();

private:
	bool OnResize(WindowResizeEvent& e);

private:
	Ref<DepthPrepass> m_DepthPrepass = nullptr;
	Ref<LightCulling> m_LightCulling = nullptr;
	Ref<FinalShading> m_FinalShading = nullptr;

	std::vector<Ref<Entity>> m_Entities = { };
	std::vector<PointLight> m_PointLights = { };

	Ref<DynamicUniformBuffer> m_ModelBuffer = nullptr;
	Ref<UniformBuffer> m_CameraBuffer = nullptr;
};