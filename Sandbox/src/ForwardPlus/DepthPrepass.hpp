#pragma once

#include <Swift/Core/Application.hpp>
#include <Swift/Core/Input/Input.hpp>
#include <Swift/Utils/Mesh.hpp>

#include <Swift/Renderer/Pipeline.hpp>
#include <Swift/Renderer/Shader.hpp>
#include <Swift/Renderer/Descriptors.hpp>

using namespace Swift;

class DepthPrepass
{
public:
	DepthPrepass(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	virtual ~DepthPrepass() = default;

	bool OnResize(WindowResizeEvent& e);

	inline Ref<Pipeline> GetPipeline() { return m_DepthPipeline; }
	inline Ref<RenderPass> GetRenderPass() { return m_DepthPass; }
	inline Ref<DescriptorSets> GetDescriptorSets() { return m_DescriptorSets; }

	static Ref<DepthPrepass> Create(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);

private:
	Ref<Pipeline> m_DepthPipeline = nullptr;
	Ref<RenderPass> m_DepthPass = nullptr;

	Ref<DescriptorSets> m_DescriptorSets = nullptr;
};
