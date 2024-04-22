#version 460 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 0, rgba8) uniform writeonly image2D u_Image;

void main() 
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);

    vec4 color = vec4(float(pixelCoords.x) / float(imageSize(u_Image).x),
                      float(pixelCoords.y) / float(imageSize(u_Image).y),
                      0.5,
                      1.0);

    imageStore(u_Image, pixelCoords, color);
}