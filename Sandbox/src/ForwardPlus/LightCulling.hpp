#pragma once

#include <Swift/Core/Application.hpp>
#include <Swift/Core/Input/Input.hpp>
#include <Swift/Utils/Mesh.hpp>

#include <Swift/Renderer/Shader.hpp>
#include <Swift/Renderer/Buffers.hpp>
#include <Swift/Renderer/Pipeline.hpp>
#include <Swift/Renderer/Descriptors.hpp>
#include <Swift/Renderer/CommandBuffer.hpp>

#include "ForwardPlus/SceneItems.hpp"

using namespace Swift;

/////////////////////////////////////////////////////////////////////////////
// Set 0
struct LightsBuffer
{
public:
	uint32_t AmountOfPointLights = 0;
	PUBLIC_PADDING(0, 12);
	std::array<PointLight, MAX_POINTLIGHTS> PointLights = { };
};

struct PointLightVisibilty
{
	uint32_t Count = 0;
	PUBLIC_PADDING(0, 12);
	std::array<uint32_t, MAX_POINTLIGHTS_PER_TILE> Indices = { };
};

struct LightVisibilityBuffer
{
public:
	uint32_t AmountOfTiles = 0;
	PUBLIC_PADDING(0, 12);
	std::vector<PointLightVisibilty> VisiblePointLights = {};
};

// Set 1
struct SceneUniform
{
public:
	glm::uvec2 ScreenSize = { };

public:
	SceneUniform(const glm::uvec2& screenSize)
		: ScreenSize(screenSize)
	{
	}
};
/////////////////////////////////////////////////////////////////////////////

struct TileCount
{
public:
	uint32_t X = 0;
	uint32_t Y = 0;

public:
	TileCount() = default;
	TileCount(uint32_t x, uint32_t y)
		: X(x), Y(y)
	{
	}
	virtual ~TileCount() = default;
};

class LightCulling
{
public:
	LightCulling(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	virtual ~LightCulling() = default;

	bool OnResize(WindowResizeEvent& e);

	const TileCount GetTileCount() const;

	inline Ref<DescriptorSets> GetDescriptorSets() { return m_DescriptorSets; }

	inline Ref<Pipeline> GetPipeline() { return m_Pipeline; }
	inline Ref<ComputeShader> GetComputeShader() { return m_ComputeShader; }
	inline Ref<CommandBuffer> GetCommandBuffer() { return m_CommandBuffer; }

	inline Ref<StorageBuffer> GetLightsBuffer() { return m_LightsBuffer; }
	inline Ref<StorageBuffer> GetLightVisibilityBuffer() { return m_LightVisibilityBuffer; }
	inline Ref<UniformBuffer> GetSceneBuffer() { return m_SceneBuffer; }

	static Ref<LightCulling> Create(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);

private:
	Ref<DescriptorSets> m_DescriptorSets = nullptr;

	Ref<Pipeline> m_Pipeline = nullptr;
	Ref<ComputeShader> m_ComputeShader = nullptr;
	Ref<CommandBuffer> m_CommandBuffer = nullptr;

	Ref<StorageBuffer> m_LightsBuffer = nullptr;
	Ref<StorageBuffer> m_LightVisibilityBuffer = nullptr;
	Ref<UniformBuffer> m_SceneBuffer = nullptr;
};
