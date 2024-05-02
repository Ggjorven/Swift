#include "LightCulling.hpp"

LightCulling::LightCulling(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
    m_LightsBuffer = StorageBuffer::Create(sizeof(LightsBuffer));

    auto& window = Application::Get().GetWindow();
    uint32_t tiles = (uint32_t)(((float)window.GetWidth() / TILE_SIZE) * ((float)window.GetHeight() / TILE_SIZE));
    m_LightVisibilityBuffer = StorageBuffer::Create((sizeof(PointLightVisibilty) * tiles) + sizeof(uint32_t) * 4); // uint32_t for amount * 4 to account for padding

    m_SceneBuffer = UniformBuffer::Create(sizeof(SceneUniform));

    m_DescriptorSets = DescriptorSets::Create({
        // Set 0
        { 1, { 0, {
            { DescriptorType::Image, 0, "u_DepthBuffer", ShaderStage::Compute },
            { DescriptorType::StorageBuffer, 1, "u_Lights", ShaderStage::Compute },
            { DescriptorType::StorageBuffer, 2, "u_Visibility", ShaderStage::Compute }
        }}},

        // Set 1
        { 1, { 1, {
            { DescriptorType::UniformBuffer, 0, "u_Camera", ShaderStage::Compute },
            { DescriptorType::UniformBuffer, 1, "u_Scene", ShaderStage::Compute }
        }}},
    });

    CommandBufferSpecification cmdBufSpecs = {};
    cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

    m_CommandBuffer = CommandBuffer::Create(cmdBufSpecs);

    ShaderSpecification shaderSpecs = {};
    shaderSpecs.Compute = cacher->GetLatest(compiler, "assets/shaders/caches/LightCulling.comp.cache", "assets/shaders/LightCulling.comp.glsl", ShaderStage::Compute);

    m_ComputeShader = ComputeShader::Create(shaderSpecs);

    m_Pipeline = Pipeline::Create({ }, m_DescriptorSets, m_ComputeShader);
}

bool LightCulling::OnResize(WindowResizeEvent& e)
{
    auto& window = Application::Get().GetWindow();
    uint32_t tiles = (uint32_t)(((float)window.GetWidth() / TILE_SIZE) * ((float)window.GetHeight() / TILE_SIZE));
    m_LightVisibilityBuffer = StorageBuffer::Create(sizeof(PointLightVisibilty) * tiles);

    return false;
}

const TileCount LightCulling::GetTileCount() const
{
    TileCount tiles = {};
    tiles.X = (Application::Get().GetWindow().GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
    tiles.Y = (Application::Get().GetWindow().GetHeight() + TILE_SIZE - 1) / TILE_SIZE;
    return tiles;
}

Ref<LightCulling> LightCulling::Create(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
    return RefHelper::Create<LightCulling>(compiler, cacher);
}
