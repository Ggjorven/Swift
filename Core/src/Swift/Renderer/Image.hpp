#pragma once

#include <optional>
#include <filesystem>

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

namespace Swift
{

	struct Descriptor;
	class Pipeline;
	class DescriptorSet;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Specifications 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Empty is for creating your own one, File is for creating an image from a file
	enum class ImageUsage : uint8_t
	{
		None = 0, Size, File
	};

	enum class ImageUsageFlags : uint8_t
	{
		None = 0, Sampled = BIT(0), Storage = BIT(1), Colour = BIT(2), Depth = BIT(3), Transient = BIT(4), Input = BIT(5),
		NoMipMaps = BIT(6) // Note(Jorben): Depth always has to have the NoMipMaps flag
	};
	DEFINE_BITWISE_OPS(ImageUsageFlags)

	enum class ImageLayout : uint32_t
	{
		Undefined = 0, General = 1, Colour = 2, Depth = 3, DepthRead = 4, ShaderRead = 5, Presentation = 1000001002, SharedPresentation = 1000111000
	};
	DEFINE_BITWISE_OPS(ImageLayout)

	enum class ImageFormat : uint8_t
	{
		None = 0, RGBA, BGRA, sRGB, Depth32SFloat, Depth32SFloatS8, Depth24UnormS8
	};

	struct ImageSpecification
	{
	public:
		ImageUsage Usage = ImageUsage::None;
		ImageUsageFlags Flags = ImageUsageFlags::Sampled;
		ImageLayout Layout = ImageLayout::ShaderRead;
		ImageFormat Format = ImageFormat::RGBA;

		std::filesystem::path Path = {};

		uint32_t Width = 0;
		uint32_t Height = 0;

	public:
		ImageSpecification() = default;
		ImageSpecification(uint32_t width, uint32_t height, ImageUsageFlags flags);
		ImageSpecification(const std::filesystem::path& path, ImageUsageFlags flags = ImageUsageFlags::Sampled | ImageUsageFlags::Colour);
		virtual ~ImageSpecification() = default;
	};

	class Image2D
	{
	public:
		Image2D() = default;
		virtual ~Image2D() = default;

		virtual void SetData(void* data, size_t size) = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void Upload(Ref<DescriptorSet> set, Descriptor element) = 0;
		virtual void Transition(ImageLayout initial, ImageLayout final) = 0;

		virtual ImageSpecification& GetSpecification() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		static Ref<Image2D> Create(const ImageSpecification& specs);
	};

}