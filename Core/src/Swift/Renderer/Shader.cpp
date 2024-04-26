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
			APP_ASSERT(false, "Failed to open file: '{0}'", path.string());
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
		{
			APP_ASSERT(false, "Failed to open file '{0}'", path.string());
			return {};
		}

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

	void ShaderCacher::Cache(const std::filesystem::path& path, const std::vector<char>& code)
	{
		std::ofstream file(path, std::ios::binary);

		if (!file.is_open() || !file.good())
		{
			APP_ASSERT(false, "Failed to open '{0}'", path.string());
			return;
		}

		file.write(code.data(), code.size());

		file.close();
	}

	std::vector<char> ShaderCacher::Retrieve(const std::filesystem::path& path)
	{
		return ShaderSpecification::ReadSPIRVFile(path);
	}

	bool ShaderCacher::CacheUpToDate(const std::filesystem::path& cache, const std::filesystem::path& shader)
	{
		if (!std::filesystem::exists(cache) || !std::filesystem::exists(shader))
		{
			APP_LOG_WARN("Failed to check if cache is up to date since file was invalid or doesn't exist yet.");
			return false;
		}

		auto cacheTime = std::filesystem::last_write_time(cache);
		auto shaderTime = std::filesystem::last_write_time(shader);

		return cacheTime >= shaderTime;
	}

	std::vector<char> ShaderCacher::GetLatest(Ref<ShaderCompiler> compiler, const std::filesystem::path& cache, const std::filesystem::path& shader, ShaderStage stage)
	{
		if (CacheUpToDate(cache, shader))
			return Retrieve(cache);

		auto result = compiler->Compile(ShaderSpecification::ReadGLSLFile(shader), stage);
		Cache(cache, result);
		return result;
	}

	Ref<ShaderCacher> ShaderCacher::Create()
	{
		return RefHelper::Create<ShaderCacher>();
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