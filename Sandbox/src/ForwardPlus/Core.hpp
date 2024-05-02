#pragma once

#include <Swift/Utils/Utils.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Model
{
public:
	glm::mat4 Matrix = glm::mat4(1.0f);

public:
	Model() = default;
	Model(const glm::mat4& model)
		: Matrix(model)
	{
	}
};

class Camera
{
public:
	glm::mat4 View = glm::mat4(1.0f);
	glm::mat4 Projection = glm::mat4(1.0f);
	glm::vec2 DepthUnpackConsts = {};
	PUBLIC_PADDING(0, 8);

public:
	Camera() = default;
	Camera(const glm::mat4& view, const glm::mat4& projection)
		: View(view), Projection(projection)
	{
	}
};