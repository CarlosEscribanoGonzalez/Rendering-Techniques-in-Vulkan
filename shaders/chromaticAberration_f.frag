#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable

layout( location = 0 ) in vec2 f_uvs;
layout ( set = 0, binding = 1 ) uniform sampler2D i_color;
layout(location = 0) out vec4 out_color;

const float INTENSITY = 0.05f;

void main() {
    vec2 center = vec2(0.5f, 0.5f);
    vec2 offset = (f_uvs - center) * INTENSITY * distance(f_uvs, center);
    float r = texture(i_color, f_uvs + offset).r;
    float g = texture(i_color, f_uvs).g;
    float b = texture(i_color, f_uvs - offset).b;
    out_color = vec4(r, g, b, 1.0);
}