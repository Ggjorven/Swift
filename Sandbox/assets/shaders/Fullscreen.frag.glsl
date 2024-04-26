#version 460 core

layout(location = 0) out vec4 o_Colour;

layout (location = 0) in vec2 v_TexCoord;

layout(set = 0, binding = 0) uniform sampler2D u_Image;

void main() 
{
    // We invert the texcoords because ShaderToy uses a different system
    o_Colour = texture(u_Image, vec2(v_TexCoord.x, 1.0 - v_TexCoord.y));
}