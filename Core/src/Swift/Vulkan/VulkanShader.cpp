#include "swpch.h"
#include "VulkanShader.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanRenderer.hpp"
#include "Swift/Vulkan/VulkanCommandBuffer.hpp"

#include <shaderc/shaderc.hpp>

namespace Swift
{

	static shaderc_shader_kind ShaderStageToShaderCType(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::Vertex:
			return shaderc_glsl_vertex_shader;
		case ShaderStage::Fragment:
			return shaderc_glsl_fragment_shader;
		case ShaderStage::Compute:
			return shaderc_glsl_compute_shader;

		default:
			APP_LOG_ERROR("Invalid shader stage passed in.");
			break;
		}

		// Return vertex shader by default.
		return shaderc_glsl_vertex_shader;
	}

	std::vector<char> VulkanShaderCompiler::Compile(const std::string& code, ShaderStage stage)
	{
		shaderc::Compiler compiler = {};
		shaderc::CompileOptions options = {};
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(code, ShaderStageToShaderCType(stage), "", options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			APP_ASSERT(false, "Error compiling shader: {0}", module.GetErrorMessage());

		// Convert SPIR-V code to vector<char>
		const uint32_t* data = module.cbegin();
		const size_t numWords = module.cend() - module.cbegin();
		const size_t sizeInBytes = numWords * sizeof(uint32_t);
		const char* bytes = reinterpret_cast<const char*>(data);

		return std::vector<char>(bytes, bytes + sizeInBytes);
	}

	ShaderSpecification VulkanShaderCompiler::Compile(const std::string& fragment, const std::string& vertex)
	{
		ShaderSpecification code = {};
		code.Fragment = Compile(fragment, ShaderStage::Fragment);
		code.Vertex = Compile(vertex, ShaderStage::Vertex);
		return code;
	}



	VulkanShader::VulkanShader(ShaderSpecification code)
	{
		if (!code.Vertex.empty())
			m_VertexShader = CreateShaderModule(code.Vertex);
		if (!code.Fragment.empty())
			m_FragmentShader = CreateShaderModule(code.Fragment);
	}

	VulkanShader::~VulkanShader()
	{
		auto fragmentShader = m_FragmentShader;
		auto vertexShader = m_VertexShader;

		Renderer::SubmitFree([fragmentShader, vertexShader]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

			vkDestroyShaderModule(device, fragmentShader, nullptr);
			vkDestroyShaderModule(device, vertexShader, nullptr);
		});
	}

	VkShaderModule VulkanShader::CreateShaderModule(const std::vector<char>& data)
	{
		auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(data.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			APP_LOG_ERROR("Failed to create shader module!");

		return shaderModule;
	}

	VulkanComputeShader::VulkanComputeShader(ShaderSpecification code)
	{
		if (!code.Compute.empty())
			m_ComputeShader = VulkanShader::CreateShaderModule(code.Compute);
	}

	VulkanComputeShader::~VulkanComputeShader()
	{
		auto computeShader = m_ComputeShader;

		Renderer::SubmitFree([computeShader]()
		{
			auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();

			vkDestroyShaderModule(device, computeShader, nullptr);
		});
	}

	void VulkanComputeShader::Dispatch(Ref<CommandBuffer> commandBuffer, uint32_t width, uint32_t height, uint32_t depth)
	{
		auto vkCommand = RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer);

		vkCmdDispatch(vkCommand->GetVulkanCommandBuffer(Renderer::GetCurrentFrame()), width, height, depth);
	}

}