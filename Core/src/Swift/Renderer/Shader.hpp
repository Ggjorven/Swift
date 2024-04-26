#pragma once

#include <array>
#include <vector>
#include <optional>
#include <filesystem>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include "Swift/Renderer/Descriptors.hpp"

namespace Swift
{

	class CommandBuffer;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Specifications 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct ShaderSpecification
	{
	public:
		std::vector<char> Fragment = { };
		std::vector<char> Vertex = { };
		std::vector<char> Compute = { };

		static std::string ReadGLSLFile(const std::filesystem::path& path);
		static std::vector<char> ReadSPIRVFile(const std::filesystem::path& path);

	public:
		ShaderSpecification() = default;
		ShaderSpecification(const std::vector<char>& fragment, const std::vector<char>& vertex);
		virtual ~ShaderSpecification() = default;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Classes 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ShaderCompiler
	{
	public:
		ShaderCompiler() = default;
		virtual ~ShaderCompiler() = default;

		virtual std::vector<char> Compile(const std::string& code, ShaderStage stage) = 0;
		virtual ShaderSpecification Compile(const std::string& fragment, const std::string& vertex) = 0;

		static Ref<ShaderCompiler> Create();
	};

	class ShaderCacher
	{
	public:
		ShaderCacher() = default;
		virtual ~ShaderCacher() = default;

		void Cache(const std::filesystem::path& path, const std::vector<char>& code);
		std::vector<char> Retrieve(const std::filesystem::path& path);

		bool CacheUpToDate(const std::filesystem::path& cache, const std::filesystem::path& shader);

		std::vector<char> GetLatest(Ref<ShaderCompiler> compiler, const std::filesystem::path& cache, const std::filesystem::path& shader, ShaderStage stage);

		static Ref<ShaderCacher> Create();
	};

	class Shader
	{
	public:
		Shader() = default;
		virtual ~Shader() = default;

		static Ref<Shader> Create(ShaderSpecification specs);
	};

	class ComputeShader
	{
	public:
		ComputeShader() = default;
		virtual ~ComputeShader() = default;

		// Note(Jorben): Make sure a pipeline with compute shader added is bound
		virtual void Dispatch(Ref<CommandBuffer> commandBuffer, uint32_t width, uint32_t height, uint32_t depth) = 0;

		static Ref<ComputeShader> Create(ShaderSpecification specs);
	};

}