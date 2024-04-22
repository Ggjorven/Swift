#pragma once

#include <filesystem>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

#include <glm/glm.hpp>

#include "Swift/Renderer/Buffers.hpp"

namespace Swift
{

	struct MeshVertex
	{
	public:
		glm::vec3 Position = { };
		glm::vec2 TexCoord = { };
		glm::vec3 Normal = { };

	public:
		static BufferLayout GetLayout();
	};

	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(const std::filesystem::path& path);
		virtual ~Mesh() = default;

		Ref<VertexBuffer>& GetVertexBuffer() { return m_VertexBuffer; }
		Ref<IndexBuffer>& GetIndexBuffer() { return m_IndexBuffer; }

		static Ref<Mesh> Create(const std::filesystem::path& path);

	private:
		Ref<VertexBuffer> m_VertexBuffer = nullptr;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;

	};

}