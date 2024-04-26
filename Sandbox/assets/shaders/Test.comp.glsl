#version 460 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 0, rgba8) uniform writeonly image2D u_Image;

layout(std140, set = 0, binding = 1) uniform SceneSettings
{
    vec2 u_MousePos;
    uint u_Width;
    uint u_Height;
    uint u_Frame;
    uint u_FPS;
    float u_DeltaTime;
    float u_Timer;
};

// To be able to use (steal) code from shadertoy
vec3  iResolution   = vec3(u_Width, u_Height, 1.0);           
float iTime         = u_Timer;                 
float iTimeDelta    = u_DeltaTime;            
float iFrameRate    = u_FPS;            
int   iFrame        = int(u_Frame);                
vec4  iMouse        = vec4(u_MousePos, 0.0, 0.0);

vec2 fragCoord = vec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
vec4 fragColor = vec4(1.0, 1.0, 1.0, 1.0);


// From: https://www.shadertoy.com/view/mtyGWy
vec3 palette(float t) 
{
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

void main()
{
    vec2 uv = (fragCoord * 2.0 - iResolution.xy) / iResolution.y;
    vec2 uv0 = uv;
    vec3 finalColor = vec3(0.0);
    
    for (float i = 0.0; i < 4.0; i++) 
    {
        uv = fract(uv * 1.5) - 0.5;

        float d = length(uv) * exp(-length(uv0));

        vec3 col = palette(length(uv0) + i*.4 + iTime*.4);

        d = sin(d*8. + iTime)/8.;
        d = abs(d);
        d = pow(0.01 / d, 1.2);

        finalColor += col * d;
    }
    fragColor = vec4(finalColor, 1.0);

    imageStore(u_Image, ivec2(gl_GlobalInvocationID.xy), fragColor);
}