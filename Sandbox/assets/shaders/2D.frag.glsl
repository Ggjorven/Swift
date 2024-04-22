#version 460 core

layout(location = 0) out vec4 o_Colour;

layout (location = 0) in vec2 v_TexCoord;

layout(set = 0, binding = 0) uniform sampler2D u_Image;

void main() 
{
    o_Colour = texture(u_Image, v_TexCoord);
}