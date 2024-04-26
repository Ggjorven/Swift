#include "swpch.h"
#include "Buffers.hpp"

#include "Swift/Core/Logging.hpp"

#include "Swift/Renderer/Renderer.hpp"
#include "Swift/Renderer/Descriptors.hpp"

#include "Swift/Vulkan/VulkanBuffers.hpp"

namespace Swift
{

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Specifications 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	uint32_t DataTypeSize(DataType type)
	{
		switch (type)
		{
		case DataType::Float:    return 4;
		case DataType::Float2:   return 4 * 2;
		case DataType::Float3:   return 4 * 3;
		case DataType::Float4:   return 4 * 4;
		case DataType::Mat3:     return 4 * 3 * 3;
		case DataType::Mat4:     return 4 * 4 * 4;
		case DataType::Int:      return 4;
		case DataType::Int2:     return 4 * 2;
		case DataType::Int3:     return 4 * 3;
		case DataType::Int4:     return 4 * 4;
		case DataType::Bool:     return 1;
		}

		APP_ASSERT(false, "Unknown DataType!");
		return 0;
	}

	BufferElement::BufferElement(DataType type, uint32_t location, const std::string& name)
		: Name(name), Location(location), Type(type), Size(DataTypeSize(type)), Offset(0)
	{
	}

	uint32_t BufferElement::GetComponentCount() const
	{
		switch (Type)
		{
		case DataType::Float:   return 1;
		case DataType::Float2:  return 2;
		case DataType::Float3:  return 3;
		case DataType::Float4:  return 4;
		case DataType::Mat3:    return 3 * 3;
		case DataType::Mat4:    return 4 * 4;
		case DataType::Int:     return 1;
		case DataType::Int2:    return 2;
		case DataType::Int3:    return 3;
		case DataType::Int4:    return 4;
		case DataType::Bool:    return 1;
		}

		APP_ASSERT(false, "Unknown DataType!");
		return 0;
	}

	BufferLayout::BufferLayout(const std::initializer_list<BufferElement>& elements)
		: m_Elements(elements)
	{
		CalculateOffsetsAndStride();
	}

	void BufferLayout::CalculateOffsetsAndStride()
	{
		size_t offset = 0;
		m_Stride = 0;
		for (auto& element : m_Elements)
		{
			element.Offset = offset;
			offset += element.Size;
			m_Stride += element.Size;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Buffers 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Ref<VertexBuffer> VertexBuffer::Create(void* data, size_t size)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanVertexBuffer>(data, size);

		default:
			APP_LOG_ERROR("Invalid API selected.");
			break;
		}

		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanIndexBuffer>(indices, count);

		default:
			APP_LOG_ERROR("Invalid API selected.");
			break;
		}

		return nullptr;
	}

	Ref<UniformBuffer> UniformBuffer::Create(size_t dataSize)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanUniformBuffer>(dataSize);

		default:
			APP_LOG_ERROR("Invalid API selected.");
			break;
		}

		return nullptr;
	}

	Ref<DynamicUniformBuffer> DynamicUniformBuffer::Create(uint32_t elements, size_t sizeOfOneElement)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanDynamicUniformBuffer>(elements, sizeOfOneElement);

		default:
			APP_LOG_ERROR("Invalid API selected.");
			break;
		}

		return nullptr;
	}

	Ref<StorageBuffer> StorageBuffer::Create(size_t dataSize)
	{
		switch (RendererSpecification::API)
		{
		case RendererSpecification::RenderingAPI::Vulkan:
			return RefHelper::Create<VulkanStorageBuffer>(dataSize);

		default:
			APP_LOG_ERROR("Invalid API selected.");
			break;
		}

		return nullptr;
	}

}