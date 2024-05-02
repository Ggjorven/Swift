#pragma once

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

namespace Swift
{

	struct Descriptor;
	class DescriptorSet;
	class CommandBuffer;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Specifications 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class DataType : uint8_t
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};
	uint32_t DataTypeSize(DataType type);

	struct BufferElement
	{
	public:
		std::string Name;
		uint32_t Location;
		DataType Type;
		uint32_t Size;
		size_t Offset;

		BufferElement() = default;
		BufferElement(DataType type, uint32_t location, const std::string& name);
		virtual ~BufferElement() = default;

		uint32_t GetComponentCount() const;
	};

	struct BufferLayout
	{
	public:
		BufferLayout() = default;
		BufferLayout(const std::initializer_list<BufferElement>& elements);
		virtual ~BufferLayout() = default;

		inline uint32_t GetStride() const { return m_Stride; }
		inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }

	private:
		void CalculateOffsetsAndStride();

	private:
		std::vector<BufferElement> m_Elements = { };
		uint32_t m_Stride = 0;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Buffers 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class VertexBuffer
	{
	public:
		VertexBuffer() = default;
		virtual ~VertexBuffer() = default;

		virtual void Bind(Ref<CommandBuffer> commandBuffer) = 0;

		static Ref<VertexBuffer> Create(void* data, size_t size);
	};

	class IndexBuffer
	{
	public:
		IndexBuffer() = default;
		virtual ~IndexBuffer() = default;

		virtual void Bind(Ref<CommandBuffer> commandBuffer) const = 0;

		virtual uint32_t GetCount() const = 0;

		static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count);
	};

	// Note(Jorben): Needs to be created after the pipeline
	class UniformBuffer
	{
	public:
		UniformBuffer() = default;
		virtual ~UniformBuffer() = default;

		virtual void SetData(void* data, size_t size, size_t offset = 0) = 0;

		virtual void Upload(Ref<DescriptorSet> set, Descriptor element) = 0;

		static Ref<UniformBuffer> Create(size_t dataSize);
	};

	class DynamicUniformBuffer
	{
	public:
		DynamicUniformBuffer() = default;
		virtual ~DynamicUniformBuffer() = default;

		virtual void SetData(void* data, size_t size) = 0;

		virtual void SetDataIndexed(uint32_t index, void* data, size_t size) = 0;
		virtual void UploadIndexedData() = 0;

		virtual uint32_t GetAmountOfElements() const = 0;
		virtual size_t GetAlignment() const = 0;

		virtual void Upload(Ref<DescriptorSet> set, Descriptor element) = 0;
		virtual void Upload(Ref<DescriptorSet> set, Descriptor element, size_t offset) = 0;

		static Ref<DynamicUniformBuffer> Create(uint32_t elements, size_t sizeOfOneElement);
	};

	class StorageBuffer
	{
	public:
		StorageBuffer() = default;
		virtual ~StorageBuffer() = default;

		virtual void SetData(void* data, size_t size, size_t offset = 0) = 0;

		virtual void* StartRetrieval() = 0;
		virtual void EndRetrieval() = 0;

		virtual size_t GetSize() const = 0;

		virtual void Upload(Ref<DescriptorSet> set, Descriptor element) = 0;

		static Ref<StorageBuffer> Create(size_t dataSize);
	};

}