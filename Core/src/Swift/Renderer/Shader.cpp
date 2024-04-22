#include "swpch.h"
#include "Shader.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanShader.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"

namespace Swift
{

	ShaderSpecification::ShaderSpecification(const std::vector<char>& fragment, const std::vector<char>& vertex)
		: Fragment(fragment), Vertex(vertex)
	{
	}

	std::string ShaderSpecification::ReadGLSLFile(const std::filesystem::path& path)
	{
		std::ifstream file(path);

		if (!file.is_open() || !file.good())
		{
			APP_LOG_ERROR("Failed to open file: '{0}'", path.string());
			return {}; // Return an empty string indicating failure
		}

		std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

		file.close();
		return content;
	}

	std::vector<char> ShaderSpecification::ReadSPIRVFile(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file.is_open() || !file.good())
			APP_LOG_ERROR("Failed to open file!");

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	Ref<ShaderCompiler> ShaderCompiler::Create()
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanShaderCompiler>();

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

	Ref<Shader> Shader::Create(ShaderSpecification specs)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanShader>(specs);

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

	Ref<ComputeShader> ComputeShader::Create(ShaderSpecification specs)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanComputeShader>(specs);

		default:
			APP_ASSERT(false, "Invalid API selected.");
			break;
		}

		return nullptr;
	}

}