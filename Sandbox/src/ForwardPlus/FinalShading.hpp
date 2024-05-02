#pragma once

#include <Swift/Core/Application.hpp>
#include <Swift/Core/Input/Input.hpp>
#include <Swift/Utils/Mesh.hpp>

#include <Swift/Renderer/Pipeline.hpp>
#include <Swift/Renderer/Shader.hpp>
#include <Swift/Renderer/Descriptors.hpp>

using namespace Swift;

class FinalShading
{
public:
	FinalShading(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	virtual ~FinalShading() = default;

	bool OnResize(WindowResizeEvent& e);

	inline Ref<Pipeline> GetPipeline() { return m_Pipeline; }
	inline Ref<RenderPass> GetRenderPass() { return m_RenderPass; }
	inline Ref<DescriptorSets> GetDescriptorSets() { return m_DescriptorSets; }

	static Ref<FinalShading> Create(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);

private:
	Ref<Pipeline> m_Pipeline = nullptr;
	Ref<RenderPass> m_RenderPass = nullptr;

	Ref<DescriptorSets> m_DescriptorSets = nullptr;
};
