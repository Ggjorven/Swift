#pragma once

#include <Swift/Core/Core.hpp>

#include <Swift/Utils/Utils.hpp>
#include <Swift/Utils/Mesh.hpp>

#include <Swift/Renderer/Image.hpp>
#include <Swift/Renderer/CommandBuffer.hpp>

#include <glm/glm.hpp>

using namespace Swift;

struct Material
{
public:
	Ref<Image2D> Image = nullptr;

public:
	Material() = default;
	Material(Ref<Image2D> image)
		: Image(image)
	{
	}
};

// Note(Jorben): We don't use a ECS system since this is a basic app
class Entity
{
public:
	Entity(Ref<Mesh> mesh, const Material& material);
	virtual ~Entity() = default;

	void BindMesh(Ref<CommandBuffer> cmdBuffer) const;

	glm::mat4 CalculateMatrix() const;

	inline glm::vec3& GetPosition() { return m_Position; }
	inline glm::vec3& GetSize() { return m_Size; }
	inline glm::vec3& GetRotation() { return m_Rotation; }
	inline Ref<Mesh> GetMesh() { return m_Mesh; }
	inline Material& GetMaterial() { return m_Material; }

	static Ref<Entity> Create(Ref<Mesh> mesh, const Material& material);

private:
	Ref<Mesh> m_Mesh = nullptr;
	Material m_Material = {};

	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_Size = { 1.0f, 1.0f, 1.0f };
	glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };
};

#define TILE_SIZE 16
#define MAX_POINTLIGHTS 1024
#define MAX_POINTLIGHTS_PER_TILE 64
struct PointLight
{
public:
	glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	float Radius = 5.0f;
	
	glm::vec3 Colour = { 1.0f, 1.0f, 1.0f };
	float Intensity = 1.0f;
};