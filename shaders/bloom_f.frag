#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable

layout( location = 0 ) in vec2 f_uvs;

layout ( set = 0, binding = 1) uniform sampler2D i_color;

layout(location = 0) out vec4 out_bloom;

const float LUMINANCE_THRESHOLD = 1.5f;
const float INTENSITY = 5.0f;

void main() {
    vec3 color = texture(i_color, f_uvs).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float t = max(luminance - LUMINANCE_THRESHOLD, 0.0);
    color *= t / (t + 1.0);
    out_bloom = vec4(color * INTENSITY, 1.0);
} 