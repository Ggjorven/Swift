#include "SceneItems.hpp"

#include <glm/gtc/matrix_transform.hpp>

Entity::Entity(Ref<Mesh> mesh, const Material& material)
	: m_Mesh(mesh), m_Material(material)
{
}

void Entity::BindMesh(Ref<CommandBuffer> cmdBuffer) const
{
	m_Mesh->GetVertexBuffer()->Bind(cmdBuffer);
	m_Mesh->GetIndexBuffer()->Bind(cmdBuffer);
}

glm::mat4 Entity::CalculateMatrix() const
{
	glm::mat4 matrix = glm::mat4(1.0f);
	matrix = glm::translate(matrix, m_Position);
	matrix = glm::scale(matrix, m_Size);
	// There is probably a better way to do rotation
	matrix = glm::rotate(matrix, m_Rotation.x, { 1.0f, 0.0f, 0.0f });
	matrix = glm::rotate(matrix, m_Rotation.y, { 0.0f, 1.0f, 0.0f });
	matrix = glm::rotate(matrix, m_Rotation.z, { 0.0f, 0.0f, 1.0f });

	return matrix;
}

Ref<Entity> Entity::Create(Ref<Mesh> mesh, const Material& material)
{
	return RefHelper::Create<Entity>(mesh, material);
}
