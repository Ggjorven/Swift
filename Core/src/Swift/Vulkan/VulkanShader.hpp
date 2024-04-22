#pragma once

#include "Swift/Renderer/Shader.hpp"

#include <vulkan/vulkan.h>

namespace Swift
{

	class VulkanShaderCompiler : public ShaderCompiler
	{
	public:
		VulkanShaderCompiler() = default;
		virtual ~VulkanShaderCompiler() = default;

		std::vector<char> Compile(const std::string& code, ShaderStage stage);
		ShaderSpecification Compile(const std::string& fragment, const std::string& vertex);
	};

	class VulkanShader : public Shader
	{
	public:
		VulkanShader(ShaderSpecification code);
		virtual ~VulkanShader();

		inline VkShaderModule& GetVertexShader() { return m_VertexShader; }
		inline VkShaderModule& GetFragmentShader() { return m_FragmentShader; }

		static VkShaderModule CreateShaderModule(const std::vector<char>& data);

	private:
		VkShaderModule m_VertexShader = VK_NULL_HANDLE;
		VkShaderModule m_FragmentShader = VK_NULL_HANDLE;
	};

	class VulkanComputeShader : public ComputeShader
	{
	public:
		VulkanComputeShader(ShaderSpecification code);
		virtual ~VulkanComputeShader();

		void Dispatch(Ref<CommandBuffer> commandBuffer, uint32_t width, uint32_t height, uint32_t depth) override;

		inline VkShaderModule& GetComputeShader() { return m_ComputeShader; }

	private:
		VkShaderModule m_ComputeShader = VK_NULL_HANDLE;
	};

}