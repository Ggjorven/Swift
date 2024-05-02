#version 460 core

layout(location = 0) out vec4 o_Colour;

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in vec3 v_Normal;

#define TILE_SIZE 16
#define MAX_POINTLIGHTS 1024
#define MAX_POINTLIGHTS_PER_TILE 64

///////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Inputs
///////////////////////////////////////////////////////////////////////
// Set 0
layout(set = 0, binding = 1) uniform sampler2D u_Albedo;

layout(std140, set = 0, binding = 2) buffer LightsBuffer
{
    uint AmountOfPointLights;
    PointLight PointLights[MAX_POINTLIGHTS];
} u_Lights;

layout(std140, set = 0, binding = 3) buffer LightVisibilityBuffer 
{
	uint AmountOfTiles;
    PointLightVisibilty VisiblePointLights[/*Amount of Tiles*/];
} u_Visibility;

// Set 1
layout(std140, set = 1, binding = 1) uniform SceneUniform 
{
    uvec2 ScreenSize;
} u_Scene;
///////////////////////////////////////////////////////////////////////

vec3 CalculatePointLight(vec3 fragPos, vec3 normal, PointLight light) 
{
    vec3 lightDir = normalize(light.Position - fragPos);
    float distance = length(light.Position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.01 * distance + 0.01 * distance * distance);

    // Lambert's cosine law
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.Colour * light.Intensity * attenuation;

    return diffuse;
}

void main() 
{
    vec3 fragPos = v_Position; // assuming v_Position is in world space
    vec3 normal = normalize(v_Normal);

    vec3 resultColor = texture(u_Albedo, v_TexCoord).rgb; // base color

    // Calculate tile index
    ivec2 tileIndex = ivec2(gl_FragCoord.xy / TILE_SIZE);
    int index = tileIndex.x + tileIndex.y * int(u_Scene.ScreenSize.x / TILE_SIZE);

    // Iterate through visible point lights for this tile
    for (uint i = 0; i < u_Visibility.VisiblePointLights[index].Count; i++) 
    {
        uint lightIndex = u_Visibility.VisiblePointLights[index].Indices[i];
        PointLight light = u_Lights.PointLights[lightIndex];
        
        // Calculate point light contribution
        resultColor += CalculatePointLight(fragPos, normal, light);
    }

    o_Colour = vec4(resultColor, 1.0);
}