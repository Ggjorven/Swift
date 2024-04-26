#version 460 core

layout (location = 0) out vec2 v_TexCoord;

void main() 
{
    v_TexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(v_TexCoord * 2.0f + -1.0f, 0.0f, 1.0f);
}