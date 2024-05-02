#version 460 core

#define TILE_SIZE 16
#define MAX_POINTLIGHTS 1024
#define MAX_POINTLIGHTS_PER_TILE 64

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

///////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////
// Camera
struct Camera
{
    mat4 View;
    mat4 Projection;
	vec2 DepthUnpackConsts;
};

// PointLight
struct PointLight
{
    vec3 Position;
    float Radius;

    vec3 Colour;
    float Intensity;
};

struct PointLightVisibilty
{
    uint Count;
    uint Indices[MAX_POINTLIGHTS_PER_TILE];
};

// Extra
struct Sphere
{
	vec3 c;	 	// Center point.
	float r;	// Radius.
};
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Inputs
///////////////////////////////////////////////////////////////////////
// Set 0
layout(set = 0, binding = 0) uniform sampler2D u_DepthBuffer;

layout(std140, set = 0, binding = 1) buffer LightsBuffer
{
    uint AmountOfPointLights;
    PointLight PointLights[MAX_POINTLIGHTS];
} u_Lights;

layout(std140, set = 0, binding = 2) buffer LightVisibilityBuffer 
{
	uint AmountOfTiles;
    PointLightVisibilty VisiblePointLights[/*Amount of Tiles*/];
} u_Visibility;

// Set 1
layout(std140, set = 1, binding = 0) uniform CameraUniform 
{
    Camera Camera;
} u_Camera;

layout(std140, set = 1, binding = 1) uniform SceneUniform 
{
    uvec2 ScreenSize;
} u_Scene;
///////////////////////////////////////////////////////////////////////

bool TestFrustumSides(vec3 c, float r, vec3 plane0, vec3 plane1, vec3 plane2, vec3 plane3)
{
	bool intersectingOrInside0 = dot(c, plane0) < r;
	bool intersectingOrInside1 = dot(c, plane1) < r;
	bool intersectingOrInside2 = dot(c, plane2) < r;
	bool intersectingOrInside3 = dot(c, plane3) < r;

	return (intersectingOrInside0 && intersectingOrInside1 &&
		intersectingOrInside2 && intersectingOrInside3);
}

// From XeGTAO
float ScreenSpaceToViewSpaceDepth(const float screenDepth)
{
	float depthLinearizeMul = u_Camera.Camera.DepthUnpackConsts.x;
	float depthLinearizeAdd = u_Camera.Camera.DepthUnpackConsts.y;
	// Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"
	return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

// Shared values between all the threads in the group
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visiblePointLightCount;
shared vec4 frustumPlanes[6];

// Shared local storage for visible indices, will be written out to the global buffer at the end
shared int visiblePointLightIndices[MAX_POINTLIGHTS];

void main()
{
    ivec2 location = ivec2(gl_GlobalInvocationID.xy);
    ivec2 itemID = ivec2(gl_LocalInvocationID.xy);
    ivec2 tileID = ivec2(gl_WorkGroupID.xy);
    ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
    uint index = tileID.y * tileNumber.x + tileID.x;

    // Initialize shared global values for depth and light count
    if (gl_LocalInvocationIndex == 0)
    {
		minDepthInt = 0xFFFFFFFF;
		maxDepthInt = 0;
		visiblePointLightCount = 0;
    }

    barrier();

    // Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
    vec2 tc = vec2(location) / u_Scene.ScreenSize;
    float linearDepth = ScreenSpaceToViewSpaceDepth(textureLod(u_DepthBuffer, tc, 0).r);

    // Convert depth to uint so we can do atomic min and max comparisons between the threads
    uint depthInt = floatBitsToUint(linearDepth);
    atomicMin(minDepthInt, depthInt);
    atomicMax(maxDepthInt, depthInt);

    barrier();

    // Step 2: One thread should calculate the frustum planes to be used for this tile
    if (gl_LocalInvocationIndex == 0)
    {
		// Convert the min and max across the entire tile back to float
		float minDepth = uintBitsToFloat(minDepthInt);
		float maxDepth = uintBitsToFloat(maxDepthInt);

		// Steps based on tile sale
		vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
		vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

		// Set up starting values for planes using steps and min and max z values
		frustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x); // Left
		frustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x); // Right
		frustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y); // Bottom
		frustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y); // Top
		frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth); // Near
		frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth); // Far

		// Transform the first four planes
		for (uint i = 0; i < 4; i++)
		{
		    frustumPlanes[i] *= u_Camera.Camera.View * u_Camera.Camera.Projection;
		    frustumPlanes[i] /= length(frustumPlanes[i].xyz);
		}

		// Transform the depth planes
		frustumPlanes[4] *= u_Camera.Camera.View;
		frustumPlanes[4] /= length(frustumPlanes[4].xyz);
		frustumPlanes[5] *= u_Camera.Camera.View;
		frustumPlanes[5] /= length(frustumPlanes[5].xyz);
    }

    barrier();

    // Step 3: Cull lights.
    // Parallelize the threads against the lights now.
    // Can handle 256 simultaniously. Anymore lights than that and additional passes are performed
    const uint threadCount = TILE_SIZE * TILE_SIZE;
    uint passCount = (u_Lights.AmountOfPointLights + threadCount - 1) / threadCount;
    for (uint i = 0; i < passCount; i++)
    {
		// Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= u_Lights.AmountOfPointLights)
		    break;

		vec4 position = vec4(u_Lights.PointLights[lightIndex].Position, 1.0f);
		float radius = u_Lights.PointLights[lightIndex].Radius;
		radius += radius * 0.3f;

		// Check if light radius is in frustum
		float distance = 0.0;
		for (uint j = 0; j < 6; j++)
		{
		    distance = dot(position, frustumPlanes[j]) + radius;
		    if (distance <= 0.0) // No intersection
				break;
		}

		// If greater than zero, then it is a visible light
		if (distance > 0.0)
		{
		    // Add index to the shared array of visible indices
		    uint offset = atomicAdd(visiblePointLightCount, 1);
		    visiblePointLightIndices[offset] = int(lightIndex);
		}
    }

    barrier();

    // One thread should fill the global light buffer
    if (gl_LocalInvocationIndex == 0)
    {
		const uint offset = index * MAX_POINTLIGHTS_PER_TILE; // Determine position in global buffer
		for (uint i = 0; i < visiblePointLightCount; i++) 
		{
			u_Visibility.VisiblePointLights[index].Indices[i] = uint(visiblePointLightIndices[i]);
		}
		u_Visibility.VisiblePointLights[index].Count = visiblePointLightCount;

		u_Visibility.AmountOfTiles = (u_Scene.ScreenSize.x + TILE_SIZE - 1) / TILE_SIZE * (u_Scene.ScreenSize.y + TILE_SIZE - 1) / TILE_SIZE;
    }
}